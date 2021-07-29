#pragma once
#include <ArduinoJson.h>
#include "Messages.h"
#include "Module.h"

class RelaysModule : public Module {
private:
  struct RelayConfig {
    uint8_t pin;
    bool    inverse;
  };

  struct RelayInfo {
    String  alias;
    uint8_t pin;
    bool    inverse;
    bool    persistent;
  };

  static const RelayConfig DEFAULT_CONFIG[];
  RelayInfo relays[Config::RELAY_MAX_RELAYS];
  uint8_t pendingStateDelay = 0;
  String pendingState;

public:
  RelaysModule();
  virtual ~RelaysModule();
  virtual void          tick_100mS( uint8_t phase );
  // Module identification
  virtual const char*   getId()  { return RELAYS_MODULE; }
  virtual const char*   getName()  { return Messages::TITLE_RELAYS_MODULE; }
  // Module Web interface
  virtual const String  getModuleWebpage();
  virtual const String  getStatusWebpage();
  // A generic getData/setData interface
  virtual const String  getString( const String& key );
  virtual ResultData    setString( const String& key, const String& value );

protected:
  virtual bool          handleCommand( const String& cmd, const String& args );
  virtual void          handleCommandResults( const String& cmd, const String& args, const String& result );
  virtual void          resolveTemplateKey( const String& key, String& out );

private:
  ResultData            buildRelayData( const String& data );
  String                getDefaultRelayData();
  String                getRelayData();
  String                getRelayName( const uint8_t index );
  void                  initializeHardware();
  void                  saveRelayValue( const uint8_t index, const RelayInfo info, const uint8_t value );
  void                  switchRelay( const uint8_t index, const RelayInfo info, const uint8_t value );
  uint8_t               toRelayIndex( const String& value );

  static String         getRelayHtmlStatus( const RelayInfo info );
  static String         getRelayId( const uint8_t index );
  static uint8_t        parseRelayPayload( const String& payload );
  static String         toJsonString( const uint8_t index, const RelayInfo info );
};