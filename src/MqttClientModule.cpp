#include <ArduinoLog.h>
#include "Config.h"
#include "Events.h"
#include "ModulesManager.h"
#include "MqttClientModule.h"
#include "Messages.h"
#include "str_switch.h"
#include "Utils.h"

/* Public */

MqttClientModule* MqttClientModule::instance = nullptr;

MqttClientModule::MqttClientModule() {
  properties.has_module_webpage = true;
  properties.has_status_webpage = true;
  properties.loop_required = true;
  properties.tick_100mS_required = true;
  flags.data = 0;
  instance = this;

  // Initialize MQTT client
  mg_mgr_init( &manager, NULL );
  // Subscribe to event bus connectivity events.
  eventBusToken = Bus.listen<ConnectivityEvent>( [this](const ConnectivityEvent& event) {
    switch( event.type ) {
      case ConnectivityEvent::TYPE_WIFI:
        if( event.connected ) {
          if( !flags.initial_start ) {
            flags.initial_start = true;
            reconnect();
          }
        }
        break;

      default:
        break;
    }
  });

  // Subscribe to event bus command response events.
  Bus.listen<CommandResponseEvent>( eventBusToken, [this](const CommandResponseEvent& event) {
    publish( TopicPrefix::STAT, event.topic, event.results );
  });
}

MqttClientModule::~MqttClientModule() {
  mg_mgr_free( &manager );
  Bus.unlisten<ConnectivityEvent>( eventBusToken );
//  disconnect();
  instance = nullptr;
}

void MqttClientModule::loop() {
  mg_mgr_poll( &manager, 0 );
}

/**
 * Checks the MQTT connection once per second. Reconnects when connection is lost.
 * Assumed that MQTT controller is enabled in the settings.
 */
void MqttClientModule::tick_100mS( uint8_t phase ) {
  if( phase == 9 && connectionState == DISCONNECTED && State.wifiConnected() ) {
    if( !retry_counter ) {
      retry_counter = getShortOption( "RetryTime", Config::MQTT_RECONNECT_TIME );
      reconnect();
    } else {
      retry_counter--;
    }
  }
}

/* Module Web interface */

const String MqttClientModule::getModuleWebpage() {
  return makeWebpage( "/module_mqtt.html" );
}

const String MqttClientModule::getStatusWebpage() {
  return makeWebpage( "/status_mqtt_client.html" );
}

/* A generic getData/setData interface */

const String MqttClientModule::getString( const String& key ) {
  SWITCH( key.c_str() ) {
    CASE( Config::KEY_EXPORT_CONFIGURATION ): {
      DynamicJsonDocument json( Config::JSON_MESSAGE_SIZE );
      json["clientid"]  = getStringOption( "ClientId", Config::MQTT_CLIENT_ID );
      json["host"]      = getStringOption( "Host", Config::MQTT_HOST );
      json["port"]      = getShortOption( "Port", Config::MQTT_PORT );
      json["user"]      = getStringOption( "User", Config::MQTT_USERNAME );
      json["pwd"]       = getStringOption( "Pwd", Config::MQTT_PASSWORD );
      json["reconnect"] = getShortOption( "RetryTime", Config::MQTT_RECONNECT_TIME );
      json["telemetry"] = getShortOption( "Telemetry", Config::MQTT_TELEMETRY_TIME );
      json["topic"]     = getStringOption( "Topic", Config::MQTT_DEVICE_TOPIC );
      json["grptopic"]  = getStringOption( "GrpTopic", Config::MQTT_GROUP_TOPIC );
      json["cmnd"]      = getStringOption( "SubPref", Config::MQTT_SUB_PREFIX );
      json["stat"]      = getStringOption( "PubPref", Config::MQTT_PUB_PREFIX );
      json["tele"]      = getStringOption( "PubPref2", Config::MQTT_PUB_PREFIX2 );
      json["fulltopic"] = getStringOption( "FullTopic", Config::MQTT_FULL_TOPIC );
      return json.as<String>();
    }
    DEFAULT_CASE:
      return Module::getString( key );
  }
}

ResultData MqttClientModule::setString( const String& key, const String& value ) {
  SWITCH( key.c_str() ) {
    CASE( Config::KEY_IMPORT_CONFIGURATION ):
      return handleConfigImport( value );
    DEFAULT_CASE:
      return Module::setString( key, value );
  }
}

/* Public non-virtual methods */

void MqttClientModule::publish( TopicPrefix prefix, const String& subtopic, const JsonDocument& json ) {
  String topic = buildTopicName( prefix, getStringOption("Topic", Config::MQTT_DEVICE_TOPIC), subtopic );
  String data;
  serializeJson( json, data );
  publish( topic, data, false );
}

void MqttClientModule::publish( TopicPrefix prefix, const String& subtopic, const String& data ) {
  // char data[JSON_MESSAGE_SIZE];
  // va_list arglist;
  // va_start( arglist, format );
  // vsnprintf( data, sizeof(data), format, arglist );
  // va_end( arglist );

  String topic = buildTopicName( prefix, getStringOption("Topic", Config::MQTT_DEVICE_TOPIC), subtopic );
  publish( topic, data, false );
}

void MqttClientModule::publish( const String& topic, const String& data, boolean retained ) {
  // Explanation of MQTT QoS
  // https://stackoverflow.com/questions/14037302/mqtt-how-to-know-which-msg-a-puback-is-for
  if( connectionState == CONNECTED ) {
    int msg_flags = MG_MQTT_QOS(0);
    if( retained ) msg_flags |= MG_MQTT_RETAIN;
    mg_mqtt_publish( connection, topic.c_str(), ++messageId, msg_flags, data.c_str(), data.length() );
    Log.verbose( "MQTT %s %s%s" CR, topic.c_str(), data.c_str(), (retained) ? " (retained)" : "" );
  } else {
    Log.verbose( "MQTT %s %s (not connected)" CR, topic.c_str(), data.c_str() );
  }
}

void MqttClientModule::reconnect() {
  if( connectionState == CONNECTED ) {
    if( !flags.reconnect_requested ) {
      flags.reconnect_requested = true;
      Log.notice( "MQTT Reconnecting..." CR );
    }
  } else if( connectionState == DISCONNECTED ) {
    connectionState = CONNECTING;

    String url = getStringOption( "Host", Config::MQTT_HOST );
    url += ':';
    url += getShortOption( "Port", Config::MQTT_PORT );
    connection = mg_connect( &manager, url.c_str(), [](struct mg_connection* nc, int ev, void* data) {
      instance->mqttEventsHandler( nc, ev, data );
    });
  }
}

/* Protected */

bool MqttClientModule::handleCommand( const String& cmd, const String& args ) {
  SWITCH( cmd.c_str() ) {
    // ==========================================
    // Enable or disable the module debug logging.
    // Value: integer - enable or disable the debug log [0..1]
    CASE( "debug" ):
      if( args.length() > 0 ) {
        setDebugLog( Utils::toBool( args.c_str() ));
      }
      handleCommandResults( cmd, args, String( getDebugLog() ));
      return true;
    // ==========================================
    CASE( "reconnect" ):
      reconnect();
      handleCommandResults( cmd, args, Messages::OK );
      return true;
    // ==========================================
    // TODO "send" command to publish the payload via MQTT
    // the payload should be in format <prefix> <data> or <prefix>/<data>
    // ==========================================
    // Unknown command
    DEFAULT_CASE:
      return false;
  }
}

ResultData MqttClientModule::handleOption( const String& key, const String& value, Options::Action action ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    CASE( "clientid" ):
      return handleStringOption( "ClientId", value, action, {NOT_EMPTY, IMPORTANT} );
    // ==========================================
    CASE( "host" ):
      if( action != Options::READ ) {
        if( !Utils::isIpAddress( value.c_str() ))  return INVALID_VALUE;
      }
      return handleStringOption( "Host", value, action, {NOT_EMPTY, IMPORTANT} );
    // ==========================================
    CASE( "port" ):
      if( action != Options::READ ) {
        uint16_t port = atoi( value.c_str() );
        if( port < 1 || port > 65535 ) return INVALID_VALUE;
      }
      return handleShortOption( "Port", value, action, IMPORTANT );
    // ==========================================
    CASE( "user" ):
      return handleStringOption( "User", value, action, {false, false} );
    // ==========================================
    CASE( "pwd" ):
      return handleStringOption( "Pwd", value, action, {false, false} );
    // ==========================================
    CASE( "reconnect" ):
      return handleShortOption( "RetryTime", value, action, false );
    // ==========================================
    CASE( "telemetry" ):
      return handleShortOption( "Telemetry", value, action, false );
    // ==========================================
    CASE( "topic" ):
      return handleStringOption( "Topic", value, action, {NOT_EMPTY, IMPORTANT} );
    // ==========================================
    CASE( "grptopic" ):
      return handleStringOption( "GrpTopic", value, action, {NOT_EMPTY, IMPORTANT} );
    // ==========================================
    CASE( "cmnd" ):
      return handleStringOption( "SubPref", value, action, {NOT_EMPTY, IMPORTANT} );
    // ==========================================
    CASE( "stat" ):
      return handleStringOption( "PubPref", value, action, {NOT_EMPTY, IMPORTANT} );
    // ==========================================
    CASE( "tele" ):
      return handleStringOption( "PubPref2", value, action, {NOT_EMPTY, IMPORTANT} );
    // ==========================================
    CASE( "fulltopic" ):
      return handleStringOption( "FullTopic", value, action, {NOT_EMPTY, IMPORTANT} );
    // ==========================================
    DEFAULT_CASE:
      return UNKNOWN_OPTION;
  }
}

void MqttClientModule::resolveTemplateKey( const String& key, String& out ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    // Module template parameters
    CASE( "MQ_TCLID" ):    out += getStringOption( "ClientId", Config::MQTT_CLIENT_ID );       break;
    CASE( "MQ_VCLID" ):    out += getMacroOption( "ClientId", Config::MQTT_CLIENT_ID );        break;
    CASE( "MQ_HOST" ):     out += getStringOption( "Host", Config::MQTT_HOST );                break;
    CASE( "MQ_USER" ):     out += getStringOption( "User", Config::MQTT_USERNAME );            break;
    CASE( "MQ_RETRY" ):    out += getShortOption( "RetryTime", Config::MQTT_RECONNECT_TIME );  break;
    CASE( "MQ_TOPIC" ):    out += getStringOption( "Topic", Config::MQTT_DEVICE_TOPIC );       break;
    CASE( "MQ_PORT" ):     out += getShortOption( "Port", Config::MQTT_PORT );                 break;
    CASE( "MQ_PWD" ):      out += getStringOption( "Pwd", Config::MQTT_PASSWORD );             break;
    CASE( "MQ_TELE" ):     out += getShortOption( "Telemetry", Config::MQTT_TELEMETRY_TIME );  break;
    CASE( "MQ_GTOPIC" ):   out += getStringOption( "GrpTopic", Config::MQTT_GROUP_TOPIC );     break;
    CASE( "MQ_PCMND" ):    out += getStringOption( "SubPref", Config::MQTT_SUB_PREFIX );       break;
    CASE( "MQ_PSTAT" ):    out += getStringOption( "PubPref", Config::MQTT_PUB_PREFIX );       break;
    CASE( "MQ_PTELE" ):    out += getStringOption( "PubPref2", Config::MQTT_PUB_PREFIX2 );     break;
    CASE( "MQ_FTOPIC" ):   out += getStringOption( "FullTopic", Config::MQTT_FULL_TOPIC );     break;
    // ==========================================
    // Status template parameters
    CASE( "MQ_VTOPIC" ):
      out += buildTopicName( TopicPrefix::CMND, getStringOption( "Topic", Config::MQTT_DEVICE_TOPIC ), "#" );
      break;
    // ==========================================
    CASE( "Title" ):
      out += Utils::formatModuleSettingsTitle( getId(), getName() );
      break;
  }
}

/* Private methods */

/**
 * Makes a MQTT topic name.
 * @param prefix One of available topic prefixes:
 *               - 0, "cmnd", the topic to send a command to this device;
 *               - 1, "stat", this device publishes it's status to this topic;
 *               - 2, "tele", this device published it's telemetry data to this topic.
 * @param topic The topic name.
 * @param subtopic The sub-topic name.
 */
String MqttClientModule::buildTopicName( TopicPrefix prefix, const String& topic, const String& subtopic ) {
  String fulltopic = getStringOption( "FullTopic", Config::MQTT_FULL_TOPIC );
  fulltopic.replace( "#PREFIX", toString( prefix ));
  fulltopic.replace( "#TOPIC", topic );

  fulltopic.replace( "#", "" );
  fulltopic.replace( "//", "/" );
  if( !fulltopic.endsWith( "/" )) {
    fulltopic += "/";
  }
  fulltopic += subtopic;
  return fulltopic;
}

void MqttClientModule::mqttEventsHandler( struct mg_connection* nc, int ev, void* p ) {
  if( ev == MG_EV_CONNECT ) {
    int code = *(int*) p;
    if( code != 0 ) {
      const String host = getStringOption( "Host", Config::MQTT_HOST );
      const uint16_t port = getShortOption( "Port", Config::MQTT_PORT );
      Log.error( "MQTT Failed to connect to %s:%d, %s" CR, host.c_str(), port, strerror(code) );
    } else {
      // Connection established
      retry_counter = getShortOption( "RetryTime", Config::MQTT_RECONNECT_TIME );

      // Initiate a MQTT handshake
      struct mg_send_mqtt_handshake_opts opts;
      memset( &opts, 0, sizeof( opts ));
      opts.keep_alive = 10;

      const String user = getStringOption( "User", Config::MQTT_USERNAME );
      const String pwd = getStringOption( "Pwd", Config::MQTT_PASSWORD );
      if( user.length() > 0 && pwd.length() > 0 ) {
        opts.user_name = user.c_str();
        opts.password = pwd.c_str();
      }

      // LWT (Last Will and Testament)
      String lwtTopic = buildTopicName( TopicPrefix::TELE, getStringOption( "Topic", Config::MQTT_DEVICE_TOPIC ), "LWT" ).c_str();
      opts.will_topic = lwtTopic.c_str();
      opts.will_message = "Offline";
      opts.flags |= MG_MQTT_WILL_RETAIN;

      mg_set_protocol_mqtt( nc );
      const String client_id = getMacroOption( "ClientId", Config::MQTT_CLIENT_ID );
      mg_send_mqtt_handshake_opt( nc, client_id.c_str(), opts );
    }
  }

  else if( ev == MG_EV_MQTT_CONNACK ) {
    struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;
    if( msg->connack_ret_code != MG_EV_MQTT_CONNACK_ACCEPTED ) {
      Log.notice( "MQTT Connection error %d" CR, msg->connack_ret_code );
    } else {
      // Subscribe to this device topics
      const String client_id = getMacroOption( "ClientId", Config::MQTT_CLIENT_ID );
      String commandTopic = buildTopicName( TopicPrefix::CMND, getStringOption( "Topic", Config::MQTT_DEVICE_TOPIC ), "#" );
      String groupTopic = buildTopicName( TopicPrefix::CMND, getStringOption( "GrpTopic", Config::MQTT_GROUP_TOPIC ), "#" );
      String deviceIdTopic = buildTopicName( TopicPrefix::CMND, client_id.c_str(), "#" );
      struct mg_mqtt_topic_expression topic_expressions[] = {
        { commandTopic.c_str(),  0 },
        { groupTopic.c_str(),    0 },
        { deviceIdTopic.c_str(), 0 }
      };
      mg_mqtt_subscribe( nc, topic_expressions, sizeof(topic_expressions) / sizeof(*topic_expressions), ++messageId );
      nc->flags |= MG_F_USER_1;
    }
  }

  // Subscription(s) acknowledged
  else if( ev == MG_EV_MQTT_SUBACK ) {
    Log.notice( "MQTT Connected" CR );
    // Dispatch the MQTT connected state
    connectionState = CONNECTED;
    const ConnectivityEvent ev = {ConnectivityEvent::TYPE_MQTT, true};
    Bus.notify( ev );

    // LWT (Last Will and Testament)
    const String lwtTopic = buildTopicName( TopicPrefix::TELE, getStringOption( "Topic", Config::MQTT_DEVICE_TOPIC ), "LWT" );
    publish( lwtTopic, "Online", true );
  }

  //else if( ev == MG_EV_MQTT_PUBACK ) {
  //  struct mg_mqtt_message *msg = (struct mg_mqtt_message*) p;
  //  Log.verbose( "MQTT Message publishing acknowledged (msg_id: %d) (len=%d) CR", msg->message_id, msg->topic.len );
  //}

  else if( ev == MG_EV_MQTT_PUBLISH ) {
    // Last part of received topic must always be the command
    struct mg_mqtt_message *msg = (struct mg_mqtt_message*) p;
    // Forward a received command to modules
    Modules.dispatchCommand( parseCommand( msg ));
  }

  else if( ev == MG_EV_POLL && nc->flags & MG_F_USER_1 ) {
    if( flags.reconnect_requested ) {
      nc->flags |= MG_F_CLOSE_IMMEDIATELY;
      flags.reconnect_requested = false;
    }
  }

  else if( ev == MG_EV_CLOSE ) {
    connection = NULL;
    if( connectionState == CONNECTED ) {
      //Log.notice( "MQTT Disconnected, retry in %d sec" CR, retry_counter );
      Log.notice( "MQTT Disconnected" CR );
    } else {
      Log.notice( "MQTT Retry in %d sec" CR, retry_counter );
    }
    // Dispatch the MQTT connected state
    connectionState = DISCONNECTED;
    const ConnectivityEvent ev = {ConnectivityEvent::TYPE_MQTT, false};
    Bus.notify( ev );
  }
}

String MqttClientModule::toString( const TopicPrefix prefix ) {
  switch( prefix ) {
    case TopicPrefix::CMND:  return getStringOption( "SubPref", Config::MQTT_SUB_PREFIX );
    case TopicPrefix::STAT:  return getStringOption( "PubPref", Config::MQTT_PUB_PREFIX );
    case TopicPrefix::TELE:  return getStringOption( "PubPref2", Config::MQTT_PUB_PREFIX2 );
    default:                 return "";
  }
}

/* Private static methods */

String MqttClientModule::parseCommand( const struct mg_mqtt_message* msg ) {
  String command;
  if( msg->topic.len > 0 ) {
    // Last part of received topic must always be the command
    int cnt = 0;
    int topic_len = msg->topic.len;
    const char* p = msg->topic.p + topic_len - 1;

    while( topic_len-- ) {
      if( *p == '/' ) {
        p++;
        if( cnt > 0 ) {
          // Reserve a capacity for topic, payload and a separator between them
          command.reserve( topic_len + msg->payload.len + 1 );
          while( cnt-- ) {
            //command += (char)tolower( *p++ );
            command += (char)( *p++ );
          }
        }
        break;
      }
      p--;
      cnt++;
    }
    // Append a separator and MQTT payload
    if( msg->payload.len > 0 ) {
      command += ' ';
      cnt = msg->payload.len;
      p = msg->payload.p;
      while( cnt-- ) {
        //command += (char)tolower( *p++ );
        command += (char)( *p++ );
      }
    }
  }
  return command;
}
