#pragma once
#include <mongoose.h>
#include "Module.h"

class WebServerModule : public Module {

/* StateFlags */

typedef union {
  uint8_t data;
  struct {
    uint8_t debug_log     : 1;
    uint8_t spare01       : 1;
    uint8_t spare02       : 1;
    uint8_t spare03       : 1;
    uint8_t spare04       : 1;
    uint8_t spare05       : 1;
    uint8_t spare06       : 1;
    uint8_t initial_start : 1;
  };
} StateFlags;

/* Session information structure. */

struct Session {
  char id[41];                // Session ID. Must be unique and hard to guess.
  double created;             // Time when the session was created and time of last activity.
                              // Used to clean up stale sessions.
  double last_used;           // Time when the session was last active.
  char* user;                 // User name this session is associated with.
};

/* WebServerModule */

private:
  int eventBusToken;
  StateFlags flags;

  mg_mgr manager;
  mg_connection* connection = NULL;
  Session sessions[Config::WEB_MAX_SESSIONS];

  static WebServerModule* instance;

public:
  WebServerModule();
  virtual ~WebServerModule();
  virtual void         loop();
  // Module identification
  virtual const char*  getId()    { return WEBSERVER_MODULE; }
  virtual const char*  getName()  { return Messages::TITLE_WEBSERVER_MODULE; }
  // Module Web interface
  virtual const String getModuleWebpage();
  // A generic getData/setData interface
  virtual const String getString( const String& key );
  virtual ResultData   setString( const String& key, const String& value );

protected:
  virtual bool         handleCommand( const String& cmd, const String& args );
  virtual ResultData   handleOption( const String& key, const String& value, Options::Action action );
  virtual void         resolveTemplateKey( const String& key, String& out );

private:
  bool                 checkPassword( const char* user, const char* pass );
  void                 checkSessions();
  Session*             createSession( const char* user, const http_message* hm );
  Session*             getSession( http_message* hm );

  void                 eventHandler( mg_connection* nc, int ev, void* ev_data );
  void                 handleGetDataRequest( mg_connection *nc, void* ev_data );
  void                 handleSetDataRequest( mg_connection *nc, void* ev_data );
  void                 handleSpiffsRequest( mg_connection *nc, int ev, void* ev_data );
  void                 handleUploadRequest( mg_connection *nc, int ev, void* ev_data );

  void                 startServer();

  static void          debugLog( const char* msg, const mg_str* s );
  static void          destroySession( Session* s );
  static void          dispatchCommand( const String& data );
  static String        dispatchSaveSettings( const mg_str* s );
  static Module*       findRequestedModule( mg_connection *nc, void* ev_data );
  static String        getRequestParameter( mg_connection *nc, http_message* hm, const char* key );
  static bool          isWebsocket( const mg_connection* nc )    { return nc->flags & MG_F_IS_WEBSOCKET; }
  static String        prepareModuleStatus( Module* const module );
  static void          saveStatusLayout( const JsonArray& data );
  static void          sendConsoleLog( mg_connection* nc, bool broadcast );
  static void          sendModuleStatus( mg_connection* nc, Module* const module );
  static void          sendStatusPage( mg_connection* nc );
  static String        toString( mg_str* mg );
};
