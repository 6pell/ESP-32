#pragma once
#include <ArduinoJson.h>
#include <mongoose.h>
#include "Module.h"
#include "Options.h"

class MqttClientModule : public Module {

public:
    enum ConnectionState{ DISCONNECTED, CONNECTING, CONNECTED };
    enum TopicPrefix    { CMND, STAT, TELE };

private:
  typedef union {
    uint8_t data;
    struct {
      uint8_t debug_log_enabled   : 1;
      uint8_t reconnect_requested : 1;
      uint8_t spare02             : 1;
      uint8_t spare03             : 1;
      uint8_t spare04             : 1;
      uint8_t spare05             : 1;
      uint8_t spare06             : 1;
      uint8_t initial_start       : 1;
    };
  } StateFlags;

  ConnectionState          connectionState = DISCONNECTED;
  StateFlags               flags;
  int                      eventBusToken;

  struct mg_mgr            manager;
  struct mg_connection*    connection = NULL;
  unsigned int             messageId = 0;
  uint8_t                  retry_counter = 0;               // A delay to retry MQTT connection (in seconds)

  static MqttClientModule* instance;

public:
  MqttClientModule();
  virtual ~MqttClientModule();
  virtual void          loop();
  virtual void          tick_100mS( uint8_t phase );
  // Module identification
  virtual const char*   getId()    { return MQTT_MODULE; }
  virtual const char*   getName()  { return Messages::TITLE_MQTT_MODULE; }
  // Module Web interface
  virtual const String  getModuleWebpage();
  virtual const String  getStatusWebpage();
  // A generic getData/setData interface
  virtual const String  getString( const String& key );
  virtual ResultData    setString( const String& key, const String& value );

  bool                  getDebugLog()  { return flags.debug_log_enabled; }
  const String          getDeviceTopic()  { return getStringOption("Topic", Config::MQTT_DEVICE_TOPIC); }
  bool                  isConnected()  { return connectionState == CONNECTED; }
  void                  publish( TopicPrefix prefix, const String& subtopic, const JsonDocument& json );
  void                  publish( TopicPrefix prefix, const String& subtopic, const String& data );
  void                  publish( const String& topic, const String& data, boolean retained );
  void                  reconnect();
  virtual void          reinitModule() { reconnect(); }
  void                  setDebugLog( bool enabled )  { flags.debug_log_enabled = enabled; }

protected:
  virtual bool          handleCommand( const String& cmd, const String& args );
  virtual ResultData    handleOption( const String& key, const String& value, Options::Action action );
  virtual void          resolveTemplateKey( const String& key, String& out );

private:
  String                buildTopicName( TopicPrefix prefix, const String& topic, const String& subtopic );
  void                  mqttEventsHandler( struct mg_connection* nc, int ev, void* data );
  String                toString( const TopicPrefix prefix );

  static String         parseCommand( const struct mg_mqtt_message* msg );
};
