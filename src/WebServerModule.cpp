#include <algorithm>
#include <vector>
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include "Events.h"
#include "LogManagerModule.h"
#include "Messages.h"
#include "ModulesManager.h"
#include "Options.h"
#include "str_switch.h"
#include "Utils.h"
#include "WebServerModule.h"

// HINT: Array to string
// String strData;
// for (char c : byteArray) strData += c;

/* Public */

WebServerModule* WebServerModule::instance = NULL;

WebServerModule::WebServerModule() {
  flags.data = 0;
  //flags.debug_log = true;
  properties.has_module_webpage = true;
  properties.loop_required = true;
  instance = this;

  for( int i = 0; i < Config::WEB_MAX_SESSIONS; i++ ) {
    WebServerModule::Session* s = &sessions[i];
    memset( s, 0, sizeof(*s) );
  }

  // Initialize mongoose
  mg_mgr_init( &manager, NULL );

  // Subscribe to event bus connectivity events.
  eventBusToken = Bus.listen<ConnectivityEvent>( [this](const ConnectivityEvent& event) {
    switch( event.type ) {
      case ConnectivityEvent::TYPE_WIFI:
        if( event.connected && !flags.initial_start ) {
          startServer();
        }
        break;
      case ConnectivityEvent::TYPE_ACCESS_POINT:
        if( event.connected && !flags.initial_start ) {
          startServer();
        }
        break;
      default:
        break;
    }
  });
  // Subscribe to event bus log update events.
  Bus.listen<LogUpdateEvent>( eventBusToken, [this](const LogUpdateEvent& event) {
    sendConsoleLog( connection, true );
  });
  // Subscribe to event bus module status update events.
  Bus.listen<StatusChangedEvent>( eventBusToken, [this](const StatusChangedEvent& event) {
    sendModuleStatus( connection, event.module );
  });
}

WebServerModule::~WebServerModule() {
  Bus.unlistenAll( eventBusToken );
  mg_mgr_free( &manager );
  instance = NULL;
}

void WebServerModule::loop() {
  mg_mgr_poll( &manager, 0 );
}

const String WebServerModule::getModuleWebpage() {
  return makeWebpage( "/module_web_server.html" );
}

const String WebServerModule::getString( const String& key ) {
  SWITCH( key.c_str() ) {
    CASE( Config::KEY_EXPORT_CONFIGURATION ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["name"] = getStringOption( "Name", Config::WEB_SERVER_NAME );
      json["port"] = getShortOption( "Port", Config::WEB_SERVER_PORT );
      json["auth"] = (bool) getByteOption( "Auth", Config::WEB_AUTH_ENABLED );
      json["user"] = getStringOption( "User", Config::WEB_AUTH_USERNAME );
      json["pwd"]  = getStringOption( "Pwd", Config::WEB_AUTH_PASSWORD );
      return json.as<String>();
    }
    DEFAULT_CASE:
      return Module::getString( key );
  }
}

ResultData  WebServerModule::setString( const String& key, const String& value ) {
  SWITCH( key.c_str() ) {
    CASE( Config::KEY_IMPORT_CONFIGURATION ):
      return handleConfigImport( value );
    DEFAULT_CASE:
      return Module::setString( key, value );
  }
}

/* Protected methods */

bool WebServerModule::handleCommand( const String& cmd, const String& args ) {
  SWITCH( cmd.c_str() ) {
    // ==========================================
    // Enables or disables the module debug log.
    // debug 0/1 - Disable/enable the debug log.
    // Returns the actual value of debug log parameter.
    CASE( "debug" ):
      if( args.length() > 0 ) {
        flags.debug_log = Utils::toBool( args.c_str() );
      }
      handleCommandResults( cmd, args, String( flags.debug_log ));
      return true;
    // ==========================================
    DEFAULT_CASE:
      return false;
  }
}

ResultData WebServerModule::handleOption( const String& key, const String& value, Options::Action action ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    CASE( "name" ):
      return handleStringOption( "Name", value, action, {OPTIONAL, false} );
    // ==========================================
    CASE( "port" ):
      return handleShortOption( "Port", value, action, false );
    // ==========================================
    CASE( "auth" ): {
      if( action != Options::READ && !Utils::isBool( value )) {
        return INVALID_VALUE;
      }
      auto rc = handleByteOption( "Auth", String( Utils::toBool( value )), action, false );
    }
    // ==========================================
    CASE( "user" ):
      return handleStringOption( "User", value, action, {OPTIONAL, false} );
    // ==========================================
    CASE( "pwd" ):
      return handleStringOption( "Pwd", value, action, {OPTIONAL, false} );
    // ==========================================
    DEFAULT_CASE:
      return UNKNOWN_OPTION;
  }
}

void WebServerModule::resolveTemplateKey( const String& key, String& out ) {
  SWITCH( key.c_str() ) {
    CASE( "SYS_NAME" ):
    CASE( "WS_NAME" ):
      out += getStringOption( "Name", Config::WEB_SERVER_NAME );
      break;

    CASE( "WS_PORT" ):
      out += String( getShortOption( "Port", Config::WEB_SERVER_PORT ));
      break;

    CASE( "WS_AUTH" ):
      out += getByteOption( "Auth", Config::WEB_AUTH_ENABLED ) ? "checked" : "";
      break;

    CASE( "WS_USER" ):
      out += getStringOption( "User", Config::WEB_AUTH_USERNAME );
      break;

    CASE( "WS_PWD" ):
      out += getStringOption( "Pwd", Config::WEB_AUTH_PASSWORD );
      break;

    CASE( "Title" ):
      out += Utils::formatModuleSettingsTitle( getId(), getName() );
      break;
  }
}

/* Private methods */

/**
 * Cleans up sessions that have been idle for too long.
 */
void WebServerModule::checkSessions() {
  double threshold = mg_time() - Config::WEB_SESSION_TTL;
  for( int i = 0; i < Config::WEB_MAX_SESSIONS; i++ ) {
    WebServerModule::Session* s = &sessions[i];
    if( s->id != 0 && s->last_used < threshold ) {
      destroySession( s );
      Log.verbose( "WS Session %s (%s) is expired" CR, s->id, s->user );
    }
  }
}

/**
 * Creates a new session for the user.
 */
WebServerModule::Session* WebServerModule::createSession( const char* user, const struct http_message* hm ) {
  // Find first available slot or use the oldest one.
  WebServerModule::Session* s = NULL;
  WebServerModule::Session* oldest = sessions;

  for( int i = 0; i < Config::WEB_MAX_SESSIONS; i++ ) {
    if( sessions[i].id == 0 ) {
      s = &sessions[i];
      break;
    }
    if( sessions[i].last_used < oldest->last_used ) {
      oldest = &sessions[i];
    }
  }
  if( s == NULL ) {
    destroySession( oldest );
    s = oldest;
  }

  // Initialize a new session.
  s->created = s->last_used = mg_time();
  s->user = strdup( user );
  // Create an ID by putting various volatiles into a pot and stirring.
  cs_sha1_ctx ctx;
  cs_sha1_init( &ctx );
  cs_sha1_update( &ctx, (const unsigned char *) hm->message.p, hm->message.len );
  cs_sha1_update( &ctx, (const unsigned char *) s, sizeof(*s) );
  unsigned char digest[20];
  cs_sha1_final( digest, &ctx );

  char* p = s->id;
  for( int i = 0; i < 20; i++ ) {
    char const byte = digest[i];
    *p++ = Utils::hex_chars[ ( byte & 0xF0 ) >> 4 ];
    *p++ = Utils::hex_chars[ ( byte & 0x0F ) >> 0 ];
  }
  *p = '\0';
  return s;
}

WebServerModule::Session* WebServerModule::getSession( struct http_message* hm ) {
  char ssid_buf[64];
  char *ssid = ssid_buf;
  WebServerModule::Session* ret = NULL;

  struct mg_str* cookie_header = mg_get_http_header( hm, "Cookie" );
  if( cookie_header != NULL && mg_http_parse_header2( cookie_header, Config::WEB_SESSION_COOKIE_NAME, &ssid, sizeof(ssid_buf) ) > 0 ) {
    for( int i = 0; i < Config::WEB_MAX_SESSIONS; i++ ) {
      if( strncmp( sessions[i].id, ssid, 40 ) == 0 ) {
        sessions[i].last_used = mg_time();
        ret = &sessions[i];
        break;
      }
    }
  }

  if( ssid != ssid_buf ) {
    free( ssid );
  }
  return ret;
}

void WebServerModule::eventHandler( struct mg_connection* nc, int ev, void* ev_data ) {
  // ===========================================================================
  // Serve HTTP requests
  if( ev == MG_EV_HTTP_REQUEST ) {
    struct http_message* hm = (struct http_message*) ev_data;
    // *************************************************************************
    // Serve HTTP GET requests
    if( mg_vcmp( &hm->method, "GET" ) == 0 ) {
      if(flags.debug_log)  debugLog( "GET", &hm->uri );
      // ==================
      // favicon.ico
      if( mg_vcmp( &hm->uri, "/favicon.ico" ) == 0 ) {
        mg_http_serve_file( nc, hm, "/spiffs/favicon.ico", mg_mk_str("image/x-icon"), mg_mk_str("") );
        return;
      }
      // ==================
      // Login page
      if( mg_vcmp( &hm->uri, "/login" ) == 0 ) {
        mg_http_serve_file( nc, hm, "/spiffs/login.html", mg_mk_str("text/html"), mg_mk_str("") );
        return;
      }
      // ==================
      // Authentication
      // Ask the user to log in if he didn't presented a valid cookie.
      if( getByteOption( "Auth", Config::WEB_AUTH_ENABLED )) {
        WebServerModule::Session* s = getSession( hm );
        if( s == NULL ) {
          mg_http_send_redirect( nc, 302, mg_mk_str("/login"), mg_mk_str(NULL) );
          nc->flags |= MG_F_SEND_AND_CLOSE;
          return;
        }
      }
      // ==================
      // Root webpage
      if( mg_vcmp( &hm->uri, "/" ) == 0 ) {
        const String page = makeWebpage( "/home.html" );
        mg_send_head( nc, 200, page.length(), "Content-Type: text/html" );
        mg_send( nc, page.c_str(), page.length() );
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      // ==================
      // Export the all modules configuration to a JSON file.
      if( mg_vcmp( &hm->uri, "/export_cfg" ) == 0 ) {
        DynamicJsonDocument doc( Config::JSON_EXPORT_CONFIG_SIZE );
        JsonObject json = doc.to<JsonObject>();
        // Assemble the complete configuration.
        Modules.iterator( [&json](Module* module) {
          const String s = module->getString( Config::KEY_EXPORT_CONFIGURATION );
          if( s.length() > 0 ) {
            json[module->getId()] = serialized( s );
          }
        });
        // Output JSON to a string
        String out;
        out.reserve( doc.memoryUsage() );
        serializeJsonPretty( doc, out );
        mg_send_head( nc, 200, out.length(), R"(Content-Disposition: attachment; filename="esp32_config.json")" );
        mg_send( nc, out.c_str(), out.length() );
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      // ==================
      // All other GET requests are not served.
      mg_http_send_error( nc, 404, NULL );
    }

    // *************************************************************************
    // Serve HTTP POST requests
    else if( mg_vcmp( &hm->method, "POST" ) == 0 ) {
      if( flags.debug_log)  debugLog( "POST", &hm->uri );
      // ==================
      // The login handler, performs a password check.
      if( mg_vcmp( &hm->uri, "/auth" ) == 0 ) {
        char user[50], pass[50];
        int ul = mg_get_http_var( &hm->body, "user", user, sizeof(user) );
        int pl = mg_get_http_var( &hm->body, "pass", pass, sizeof(pass) );
        if( ul > 0 && pl > 0 ) {
          if( checkPassword( user, pass )) {
            WebServerModule::Session* s = createSession( user, hm );

            mg_send_response_line( nc, 200,
                            "Content-Type: text/plain\r\n"
                            "Connection: close" );
            mg_printf( nc, "Set-Cookie: %s=%s; path=/\r\n\r\n", Config::WEB_SESSION_COOKIE_NAME, s->id );
            mg_printf( nc, "302root" );

            Log.verbose( "WS '%s' is logged in, ssid=%s" CR, s->user, s->id );
          } else {
            mg_http_send_error( nc, 200, "Wrong credentials" );
          }
        } else {
          mg_http_send_error( nc, 200, "Username and password are required" );
        }
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      // ==================
      // Ask the user to log in if he didn't presented a valid cookie.
      if( getByteOption( "Auth", Config::WEB_AUTH_ENABLED )) {
        WebServerModule::Session* s = getSession( hm );
        if( s == NULL ) {
          mg_http_send_redirect( nc, 302, mg_mk_str("/login"), mg_mk_str(NULL) );
          nc->flags |= MG_F_SEND_AND_CLOSE;
          return;
        }
      }
      // ==================
      // Handles a request to obtain the list of program modules.
      // Request: HTTP_POST /modules
      // Response: [{"id":"module_id","name":"module_name"},{..}]
      if( mg_vcmp( &hm->uri, "/modules" ) == 0 ) {
        // Prepare a json array
        StaticJsonDocument<Config::JSON_CONFIG_SIZE> json;
        JsonArray array = json.to<JsonArray>();
        // First add core modules.
        uint8_t count = sizeof(CORE_MODULE_IDS) / sizeof(CORE_MODULE_IDS[0]);
        for( uint8_t i = 0; i < count; i++ ) {
          const char* id = CORE_MODULE_IDS[i];
          Module* module = Modules.get( id );
          if( module != nullptr ) {
            const Module::Properties properties = module->getProperties();
            if( properties.has_module_webpage ) {
              JsonObject nested = array.createNestedObject();
              nested["id"] = module->getId();
              nested["name"] = module->getName();
            }
          }
        }
        // Next add custom modules.
        count = sizeof(CUSTOM_MODULE_IDS) / sizeof(CUSTOM_MODULE_IDS[0]);
        for( uint8_t i = 0; i < count; i++ ) {
          const char* id = CUSTOM_MODULE_IDS[i];
          Module* module = Modules.get( id );
          if( module != nullptr ) {
            const Module::Properties properties = module->getProperties();
            if( properties.has_module_webpage ) {
              JsonObject nested = array.createNestedObject();
              nested["id"] = module->getId();
              nested["name"] = module->getName();
            }
          }
        }
        // Send the json response
        char buffer[600];
        int content_length = serializeJson( array, buffer, sizeof(buffer) );
        mg_send_head( nc, 200, content_length, "Content-Type: application/json" );
        mg_send( nc, buffer, content_length );
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      // ==================
      // Handles a request to show a module webpage, typically the module settings.
      // Request: HTTP_POST /module
      // Request parameters: "module_id" - String, the module ID.
      // Response: The module HTML page.
      if( mg_vcmp( &hm->uri, "/module" ) == 0 ) {
        // Obtain the module_id parameter from the request.
        Module* module = findRequestedModule( nc, ev_data );
        if( module ) {
          // The module should have a webpage.
          Module::Properties properties = module->getProperties();
          if( !properties.has_module_webpage ) {
            mg_send_response_line( nc, 200,
                            "Content-Type: text/plain\r\n"
                            "Connection: close" );
            mg_printf( nc, "The module %s doesn't have a webpage", module->getName() );
          } else {
            // The module has a webpage, send it.
            const String page = module->getModuleWebpage();
            if( page.length() > 0 ) {
              mg_send( nc, page.c_str(), page.length() );
            } else {
              mg_printf( nc, "Module webpage is empty" );
            }
          }
        }
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      // ==================
      // Handles a request to show the modules status webpage.
      // The modules status is build using a bootstrap4 card-columns feature. It
      // means that each module must provide is HTML content wrapped in a well-written
      // bootstrap4 card.
      // Request: HTTP_GET /status
      // Response: The modules overall status HTML page.
      if( mg_vcmp( &hm->uri, "/status" ) == 0 ) {
        sendStatusPage( nc );
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      // ==================
      // Handles a request to show the console
      // Request: HTTP_POST /console.
      // Response: The console HTML page.
      if( mg_vcmp( &hm->uri, "/console" ) == 0 ) {
        String page = makeWebpage( "/console.html" );
        mg_send_head( nc, 200, page.length(), "Content-Type: text/html" );
        mg_send( nc, page.c_str(), page.length() );
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      // ==================
      // Handles a request to save a module settings.
      // Request: HTTP_POST /save_settings.
      // Request parameters:
      // Response: Resulting message.
      if( mg_vcmp( &hm->uri, "/save_settings" ) == 0 ) {
        const String msg = dispatchSaveSettings( &hm->body );
        mg_http_send_error( nc, 200, msg.c_str() );
        return;
      }
      // ==================
      // Handles a request to obtain some module data.
      // Request: HTTP_POST /getdata.
      // Request parameters:
      //   module_id: String, the module ID.
      //   key: String, the requested data key.
      // Response: The requested data as string (HTTP 200) or an error message (HTTP <> 200).
      if( mg_vcmp( &hm->uri, "/getdata" ) == 0 ) {
        handleGetDataRequest( nc, ev_data );
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      // ==================
      // Handles a request to set/store some module data.
      // Request: HTTP_POST /setdata.
      // Request parameters:
      //   module_id: String, the module ID.
      //   key: String, the data key.
      //   value: String, the value;
      // Response: Resulting message.
      if( mg_vcmp( &hm->uri, "/setdata" ) == 0 ) {
        handleSetDataRequest( nc, ev_data );
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      // ==================
      // All other POST requests are not served.
      mg_http_send_error( nc, 404, NULL );
    }
  }

  // ***************************************************************************
  // WEBSOCKET
  else if( ev == MG_EV_WEBSOCKET_FRAME ) {
    struct websocket_message* wm = (struct websocket_message*) ev_data;

    // WebSocket TEXT message
    // TODO? Maybe it's also need to check the websocket 'final' flag? Because
    // long messages could be split into multiple frames.
    if( wm->flags & WEBSOCKET_OP_TEXT ) {
      // Convert websocket message into null-terminated characters sequence
      char buf[Config::JSON_MESSAGE_SIZE];
      unsigned int len = std::min( (unsigned int)wm->size, Config::JSON_MESSAGE_SIZE-1 );
      memcpy( buf, wm->data, len );
      buf[len] = '\0';

      // Try to decode text message as JSON object
      if( buf[0] == '{' ) {
        StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
        DeserializationError rc = deserializeJson( json, buf );
        if( rc == DeserializationError::Ok ) {
          const String type = json["type"];
          // Handle console messages (JSON format)
          if( type == "console" ) {
            const String what = json["what"];
            SWITCH( what.c_str() ) {
              // ==============================
              CASE( "reload" ):
                sendConsoleLog( nc, false );
                break;
            }
          }
          // Handle status page messages (JSON format)
          else if( type == "status" ) {
            const String what = json["what"];
            SWITCH( what.c_str() ) {
              // ==============================
              CASE( "reorder" ): {
                saveStatusLayout( json["data"] );
                break;
              }
            }
          }
          // Handle commands (JSON format)
          else if( type == "cmd" ) {
            String payload = json["payload"];
            payload.trim();
            dispatchCommand( payload );
          }
        }
      }
    }
  }

  // ===========================================================================
  // TIMER
  // Perform a session maintenance.
  else if( ev == MG_EV_TIMER ) {
    checkSessions();
    mg_set_timer( nc, mg_time() + Config::WEB_SESSION_CHECK_INTERVAL );
  }
}

/**
 * Handles a request to obtain some data from the specified module.
 * The request must contains the following parameters:
 * module_id: String, the module ID.
 * key: String, the requested data key.
 */
void WebServerModule::handleGetDataRequest( mg_connection *nc, void* ev_data ) {
  http_message* hm = (http_message*) ev_data;
  Module* module = findRequestedModule( nc, ev_data );
  String key = getRequestParameter( nc, hm, "key" );
  if( module && key.length() > 0 ) {
    // Ask the module to provide the requested string data by provided key.
    const String data = module->getString( key );
    mg_send_head( nc, 200, data.length(), "" );
    mg_send( nc, data.c_str(), data.length() );
  }
}

void WebServerModule::handleSetDataRequest( mg_connection *nc, void* ev_data ) {
  http_message* hm = (http_message*) ev_data;
  Module* module = findRequestedModule( nc, ev_data );
  String key = getRequestParameter( nc, hm, "key" );
  String value = getRequestParameter( nc, hm, "value" );
  if( module && key.length() > 0 ) {
    module->setString( key, value );
    mg_http_send_error( nc, 200, Messages::SETTINGS_SAVED_OK );
  }
}

void WebServerModule::handleSpiffsRequest( struct mg_connection *nc, int ev, void* ev_data ) {
  if( ev == MG_EV_HTTP_REQUEST ) {
    struct http_message* hm = (struct http_message*) ev_data;
    // ==================
    // Serve GET requests
    if( mg_vcmp( &hm->method, "GET" ) == 0 ) {
      if(flags.debug_log)  debugLog( "GET", &hm->uri );
      // Determine the mime type of requested file.
      String path = toString( &hm->uri );
      mg_str mime;
      if( path.endsWith( ".css" )) {
        mime = mg_mk_str( "text/css" );
      } else if( path.endsWith( ".js" )) {
        mime = mg_mk_str( "text/javascript" );
      } else if( path.endsWith( ".ico" )) {
        mime = mg_mk_str( "image/x-icon" );
      } else {
        mime = mg_mk_str( "text/plain" );
      }
      // Serve the request.
      mg_http_serve_file( nc, hm, path.c_str(), mime, mg_mk_str("") );
    }
  }
}

void WebServerModule::handleUploadRequest( struct mg_connection* nc, int ev, void* ev_data ) {
  switch( ev ) {
    case MG_EV_HTTP_MULTIPART_REQUEST: {
      http_message* const hm = (http_message*) ev_data;

      // Obtain the module ID, the kind of upload (what) and an uploaded file size.
      // These params are provided in the request header.
      const String id = toString( mg_get_http_header( hm, "X-Module" ));
      const String what = toString( mg_get_http_header( hm, "X-What" ));
      const String fsize = toString( mg_get_http_header( hm, "X-FileSize" ));
      if(flags.debug_log){
        Log.verbose( "WS id=%s what=%s" CR, id.c_str(), what.c_str() );
      }
      // Find the target module
      Module* module = Modules.get( id );
      if( !module ) {
        mg_printf( nc, "HTTP/1.1 400 Module %s is not found\r\nContent-Length: 0\r\n\r\n", id.c_str() );
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      // Check the upload size.
      const int data_size = fsize.toInt();
      if( data_size <= 0 ) {
        mg_printf( nc, "%s",
                    "HTTP/1.1 400 Invalid upload size\r\n"
                    "Content-Length: 0\r\n\r\n" );
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      // Save a pointer to active module. Forward an upload action to that module.
      nc->user_data = module;
      ResultData results = module->onDataUploadBegin( what, data_size );
      if( flags.debug_log ) {
        Log.verbose( "WS begin upload rc=%d msg=%s" CR, results.code, results.details.c_str() );
      }
      if( results.code != RC_OK ) {
        mg_printf( nc, "HTTP/1.1 500 %s\r\nContent-Length: 0\r\n\r\n", results.details.c_str() );
        nc->flags |= MG_F_SEND_AND_CLOSE;
        return;
      }
      break;
    }

    case MG_EV_HTTP_PART_DATA: {
      mg_http_multipart_part* const mp = (mg_http_multipart_part*) ev_data;
      if( nc->user_data != NULL ) {
        Module* module = (Module*) nc->user_data;
        if( mp->status < 0 ) {
          // Network error
          module->onDataUploadEnd( false );
          if( flags.debug_log ) {
            Log.error( "WS connection is closed" CR, mp->status );
          }
          nc->flags |= MG_F_SEND_AND_CLOSE;
        } else {
          ResultData results = module->onDataUploadNextBlock( mp->data.p, mp->data.len );
          if( flags.debug_log ) {
            Log.verbose( "WS upload next size=%d rc=%d msg=%s" CR, mp->data.len, results.code, results.details.c_str() );
          }
          if( results.code != RC_OK ) {
            mg_printf( nc, "HTTP/1.1 500 %s\r\nContent-Length: 0\r\n\r\n", results.details.c_str() );
            nc->flags |= MG_F_SEND_AND_CLOSE;
          }
        }
      }
      break;
    }

    case MG_EV_HTTP_PART_END: {
      mg_http_multipart_part* const mp = (mg_http_multipart_part*) ev_data;
      if( nc->user_data != NULL ) {
        Module* module = (Module*) nc->user_data;
        if( mp->status < 0 ) {
          // Network error
          module->onDataUploadEnd( false );
          if( flags.debug_log ) {
            Log.error( "WS connection is closed" CR );
          }
          nc->flags |= MG_F_SEND_AND_CLOSE;
        } else {
          ResultData results = module->onDataUploadEnd( true );
          if( flags.debug_log ) {
            Log.verbose( "WS upload end rc=%d msg=%s" CR, results.code, results.details.c_str() );
          }
          if( results.code != RC_OK ) {
            mg_printf( nc, "HTTP/1.1 500 %s\r\nContent-Length: 0\r\n\r\n", results.details.c_str() );
          } else {
            mg_printf( nc, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" );
          }
          mg_printf( nc, results.details.c_str() );
          nc->flags |= MG_F_SEND_AND_CLOSE;
        }
      }
      break;
    }
  }
}

void WebServerModule::startServer() {
  flags.initial_start = true;
  // setup and start the webserver
  const String port = String( getShortOption( "Port", Config::WEB_SERVER_PORT ));
  connection = mg_bind( &manager, port.c_str(), [](mg_connection* nc, int ev, void* data) {
    instance->eventHandler( nc, ev, data );
  });
  mg_register_http_endpoint( connection, "/spiffs", [](mg_connection* nc, int ev, void* p) {
    instance->handleSpiffsRequest( nc, ev, p );
  });
  mg_register_http_endpoint( connection, "/upload", [](mg_connection* nc, int ev, void* p) {
    instance->handleUploadRequest( nc, ev, p );
  });
  mg_set_protocol_http_websocket( connection );
}

/* Private static methods */

/**
 * A simple password check function.
 * All users should have a password defined in options.
 */
bool WebServerModule::checkPassword( const char* user, const char* pass ) {
  const String username = getStringOption( "User", Config::WEB_AUTH_USERNAME );
  const String password = getStringOption( "Pwd", Config::WEB_AUTH_PASSWORD );
  return (strcmp( user, username.c_str() ) == 0) &&
         (strcmp( pass, password.c_str() ) == 0);
}

void WebServerModule::debugLog( const char* msg, const mg_str* s ) {
  // Convert mg_str into null-terminated char sequence
  char buf[512];
  int len = s->len < 511 ? s->len : 511;
  memcpy( buf, s->p, len );
  buf[len] = '\0';
  // Output a string to the log
  Log.verbose( "WS %s %s" CR, msg, buf );
}

/**
 * Destroys the session state.
 */
void WebServerModule::destroySession( Session* s ) {
  free( s->user );
  memset( s, 0, sizeof(*s) );
}

void WebServerModule::dispatchCommand( const String& data ) {
  if( data.length() > 0 ) {
    Modules.dispatchCommand( data );
  }
}

String WebServerModule::dispatchSaveSettings( const mg_str* body ) {
  char key[Config::MAX_STRING_KEY_SIZE+1];
  char value[Config::MAX_STRING_LINE_SIZE+1];
  std::map<String, String> map;

  mg_str src = {body->p, body->len};
  uint8_t key_len = 0;
  bool skipUntilNextKey = false;

  // Parse the request body into a key-value map.
  while( src.len-- ) {
    char c = *src.p++;
    if( c == '=' ) {
      key[key_len] = '\0';
      mg_get_http_var( body, key, value, Config::MAX_STRING_LINE_SIZE );
      map.insert( std::pair<String,String>( String(key), String(value) ));
      skipUntilNextKey = true;
    } else if( c == '&' ) {
      key_len = 0;
      skipUntilNextKey = false;
    } else if( !skipUntilNextKey ) {
      if( key_len < Config::MAX_STRING_KEY_SIZE ) {
        key[key_len++] = c;
      }
    }
  }

  // Obtain the program module by it's id.
  // Forward the settings map to the module, it should handle and save them.
  try {
    const String id = map.at( "module_id" );
    Module* module = Modules.get( id );
    if( module ) {
      const ResultData result = module->dispatchSettings( map );
      // Post-process retcodes.
      if( result.code == RC_OK_RESTART ) {
        Bus.notify( (SystemEvent) {SystemEvent::TYPE_PENDING_RESTART} );
      }
      else if( result.code == RC_OK_REINIT ) {
        module->reinitModule();
      }
      return result.details;
    } else {
      // Error: module is not found.
      char msg[64];
      snprintf( msg, sizeof(msg), Messages::MODULE_NOT_FOUND, id.c_str() );
      return msg;
    }
  }
  catch( const std::out_of_range& ) {
    // Error: module_id is missed, can't determine the module.
    return Messages::MODULE_ID_MISSED;
  }
}

Module* WebServerModule::findRequestedModule( mg_connection *nc, void* ev_data ) {
  http_message* hm = (struct http_message*) ev_data;
  char id[16];
  // Obtain the module_id parameter from the request.
  int rc = mg_get_http_var( &hm->body, "module_id", id, sizeof(id) );
  if( rc <= 0 ) {
    mg_http_send_error( nc, 400, Messages::MODULE_ID_MISSED );
    return NULL;
  }
  // Get a module by it's ID.
  Module* module = Modules.get( id );
  if( !module ) {
    char msg[64];
    snprintf( msg, sizeof(msg), "Unknown module %s", id );
    mg_http_send_error( nc, 404, msg );
    return NULL;
  }
  return module;
}

String WebServerModule::getRequestParameter( mg_connection *nc, http_message* hm, const char* key ) {
  char buffer[Config::JSON_CONFIG_SIZE];
  // Obtain the data key.
  int rc = mg_get_http_var( &hm->body, key, buffer, sizeof(buffer) );
  if( rc <= 0 ) {
    String msg = Messages::REQUEST_PARAMETER_MISSED;
    msg += key;
    mg_http_send_error( nc, 400, msg.c_str() );
    return "";
  }
  return buffer;
}

String WebServerModule::prepareModuleStatus( Module* const module ) {
  String output;
  const String html = module->getStatusWebpage();
  DynamicJsonDocument json = DynamicJsonDocument( html.length() + 64 );
  json["type"] = "status";
  json["card"] = String( module->getId() );
  json["html"] = html.c_str();
  serializeJson( json, output );
  return output;
}

void WebServerModule::saveStatusLayout( const JsonArray& data ) {
  const uint8_t count = data.size();
  for( uint8_t i = 0; i < count; i++ ) {
    const String id = data.getElement( i );
    Modules.execute( id, [i](Module* module) {
      module->setByte( "StatusPos", i );
    });
  }
}

void WebServerModule::sendConsoleLog( struct mg_connection* nc, bool broadcast ) {
  if( nc == NULL ) return;
  Modules.execute( LOG_MODULE, [nc,broadcast](Module* module) {
    // Prepare a JSON message
    DynamicJsonDocument json( Config::JSON_CONFIG_SIZE * 2 );
    json["type"] = "console";
    json["payload"] = ((LogManagerModule*) module)->getLogString();
    const String msg = json.as<String>();
    // Send the message
    if( broadcast ) {
      struct mg_connection* c;
      for( c = mg_next(nc->mgr, NULL); c != NULL; c = mg_next(nc->mgr, c) ) {
        // Send log to all websocket connections, but not to the sender
        if( c == nc || !isWebsocket(c) ) {
          continue;
        }
        mg_send_websocket_frame( c, WEBSOCKET_OP_TEXT, msg.c_str(), msg.length() );
      }
    } else {
      mg_send_websocket_frame( nc, WEBSOCKET_OP_TEXT, msg.c_str(), msg.length() );
    }
  });
}

void WebServerModule::sendModuleStatus( struct mg_connection* nc, Module* const module ) {
  if( nc == NULL || module == NULL || !module->getProperties().has_status_webpage ) return;

  String out;
  struct mg_connection* c;
  for( c = mg_next(nc->mgr, NULL); c != NULL; c = mg_next(nc->mgr, c) ) {
    // Send a JSON message to all websocket connections
    if( !isWebsocket(c) ) {
      continue;
    }
    if( out.length() == 0 ) {
      out = prepareModuleStatus( module );
    }
    mg_send_websocket_frame( c, WEBSOCKET_OP_TEXT, out.c_str(), out.length() );
  }
}

void WebServerModule::sendStatusPage( struct mg_connection* nc ) {
  const uint8_t count = Modules.count();
  if( count == 0 ) {
    // No modules
    mg_send_response_line( nc, 200,
                      "Content-Type: text/html\r\n"
                      "Connection: close" );
    mg_printf( nc, "No modules are defined" );
  } else {
    // Make a dynamic list of pointers to modules that has status widgets.
    std::vector<Module*> list;
    Modules.iterator( [&list](Module* module) {
      const Module::Properties properties = module->getProperties();
      if( properties.has_status_webpage ) {
        list.push_back( module );
      }
    });
    // Sort the list by module position.
    std::sort( list.begin(), list.end(), [](Module* const a, Module* const b) -> bool {
      return a->getByte( "StatusPos" ) < b->getByte( "StatusPos" );
    });
    // Assemble a div container
    mg_send_head( nc, 200, -1, "Content-Type: text/html" );
    mg_printf_http_chunk( nc, "%s", "<div id=\"cards\" class=\"card-columns\">" );
    for( auto const& module : list ) {
      // html card
      const String page = module->getStatusWebpage();
      if( page.length() > 0 ) {
        mg_printf_http_chunk( nc, "<div class=\"card\" id=\"%s\" draggable=\"true\">", module->getId() );
        mg_send_http_chunk( nc, page.c_str(), page.length() );
        mg_printf_http_chunk( nc, "%s", "</div>" );
      }
    }
    mg_printf_http_chunk( nc, "%s", "</div><script src=\"/spiffs/drag_drop.js\"></script>" );
    // // Send status page javascript (drag&drop, etc)
    // mg_send_http_chunk( nc, (const char*)status_js_html, sizeof( status_js_html ));
    // Send empty chunk, that means an end of response
    mg_send_http_chunk( nc, "", 0 );
  }
}

String WebServerModule::toString( mg_str* mg ) {
  String s;
  const char* p = mg->p;
  int len = mg->len;

  if( len > 0 ) {
    s.reserve( len );
    do {
      s += *p++;
    } while( --len );
  }
  return s;
}