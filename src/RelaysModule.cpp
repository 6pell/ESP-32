#include <ArduinoJson.h>
#include "config.h"
#include "Events.h"
#include "Options.h"
#include "RelaysModule.h"
#include "str_switch.h"
#include "Utils.h"

const RelaysModule::RelayConfig RelaysModule::DEFAULT_CONFIG[] = {
  {Config::RELAY1_PIN, Config::RELAY1_INVERSE_PIN},
  {Config::RELAY2_PIN, Config::RELAY2_INVERSE_PIN},
  {Config::RELAY3_PIN, Config::RELAY3_INVERSE_PIN},
  {Config::RELAY4_PIN, Config::RELAY4_INVERSE_PIN},
  {Config::RELAY5_PIN, Config::RELAY5_INVERSE_PIN},
  {Config::RELAY6_PIN, Config::RELAY6_INVERSE_PIN},
  {Config::RELAY7_PIN, Config::RELAY7_INVERSE_PIN},
  {Config::RELAY8_PIN, Config::RELAY8_INVERSE_PIN},
};

/* Public */

RelaysModule::RelaysModule() {
  properties.has_module_webpage = true;
  properties.has_status_webpage = true;
  properties.tick_100mS_required = true;

  const ResultData rc = buildRelayData( getRelayData() );
  if( rc.code != RC_OK ) {
    Log.error( "REL %s" CR, rc.details );
    buildRelayData( getDefaultRelayData() );    // Failsafe
  }
  initializeHardware();
}

RelaysModule::~RelaysModule() {
  // Do nothing here.
}

void RelaysModule::tick_100mS( uint8_t phase ) {
  if( pendingStateDelay > 0 ) {
    if( --pendingStateDelay == 0 ) {
      Bus.notify( (StatusChangedEvent) {this,pendingState} );
      pendingState = "";
    }
  }
}

// Module Web interface

const String RelaysModule::getModuleWebpage() {
  return makeWebpage( "/module_relays.html" );
}

const String RelaysModule::getStatusWebpage() {
  return makeWebpage( "/status_relays.html" );
}

// A generic getData/setData interface

const String RelaysModule::getString( const String& key ) {
  SWITCH( key.c_str() ) {
    // Get the module export config (JSON string)
    // Get the relays data (JSON array string)
    CASE( Config::KEY_EXPORT_CONFIGURATION ):
    CASE( "RelayData" ): {
      return getRelayData();
    }
    DEFAULT_CASE:
      return Module::getString( key );
  }
}

ResultData RelaysModule::setString( const String& key, const String& value ) {
  SWITCH( key.c_str() ) {
    // Permanently store the new relay config (JSON array string).
    CASE( Config::KEY_IMPORT_CONFIGURATION ):
    CASE( "RelayData" ): {
      const ResultData rc = buildRelayData( value );
      if( rc.code != RC_OK ) {
        buildRelayData( getRelayData() );
      } else {
        setStringOption( "Data", value );
      }
      initializeHardware();
      return rc;
    }
    DEFAULT_CASE:
      return Module::setString( key, value );
  }
}

/* Virtual protected */

bool RelaysModule::handleCommand( const String& cmd, const String& args ) {
  SWITCH( cmd.c_str() ) {
    // ==========================================
    // Handle commands like "relays rel1 toggle"
    DEFAULT_CASE: {
      const uint8_t index = toRelayIndex( cmd );
      if( index >= 0 && index < Config::RELAY_MAX_RELAYS ) {
        const RelayInfo info = relays[index];
        if( args.length() > 0 ) {
          // A command to change the relay state.
          const uint8_t v = parseRelayPayload( args );
          switchRelay( index, info, v );
          // Send the status changed event to the EventBus.
          // The new relay state (in a JSON format) is provided as the event payload.
          const String state = toJsonString( index, info );
          pendingState = state;
          pendingStateDelay = 7;
          //Bus.notify( (StatusChangedEvent) {this,state} );
          const String topic = info.alias.length() > 0 ? info.alias : cmd;
          handleCommandResults( topic, args, state );
          return true;
        } else {
          // A command to ask, not modify the relay state.
          const String topic = info.alias.length() > 0 ? info.alias : cmd;
          handleCommandResults( topic, args, toJsonString( index, info ));
          return false;
        }
      }
    }
    return false;
  }
}

/**
 * This overridden results handler sends the relay ID instead of module ID as an event topic.
 */
void RelaysModule::handleCommandResults( const String& cmd, const String& args, const String& result ) {
  const String s = Utils::toResultsJson( cmd, args, result );
  Bus.notify<CommandResponseEvent>( (CommandResponseEvent) {this, cmd, s} );
}

/**
 */
void RelaysModule::resolveTemplateKey( const String& key, String& out ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    // Module template parameters
    CASE( "ID" ):
      out += getId();
      break;
    CASE( "Title" ):
      out += Utils::formatModuleSettingsTitle( getId(), getName() );
      break;
    // ==========================================
    // Relay ID options
    CASE( "ID1" ):  out += getRelayName( 0 );  break;
    CASE( "ID2" ):  out += getRelayName( 1 );  break;
    CASE( "ID3" ):  out += getRelayName( 2 );  break;
    CASE( "ID4" ):  out += getRelayName( 3 );  break;
    CASE( "ID5" ):  out += getRelayName( 4 );  break;
    CASE( "ID6" ):  out += getRelayName( 5 );  break;
    CASE( "ID7" ):  out += getRelayName( 6 );  break;
    CASE( "ID8" ):  out += getRelayName( 7 );  break;
    // ==========================================
    // Relay status, a per-relay buttons style to indicate relays states (ON/OFF/Disabled)
    CASE( "ST1" ):   out += getRelayHtmlStatus( relays[0] );  break;
    CASE( "ST2" ):   out += getRelayHtmlStatus( relays[1] );  break;
    CASE( "ST3" ):   out += getRelayHtmlStatus( relays[2] );  break;
    CASE( "ST4" ):   out += getRelayHtmlStatus( relays[3] );  break;
    CASE( "ST5" ):   out += getRelayHtmlStatus( relays[4] );  break;
    CASE( "ST6" ):   out += getRelayHtmlStatus( relays[5] );  break;
    CASE( "ST7" ):   out += getRelayHtmlStatus( relays[6] );  break;
    CASE( "ST8" ):   out += getRelayHtmlStatus( relays[7] );  break;
  }
}

/* Private */

ResultData RelaysModule::buildRelayData( const String& data ) {
  DynamicJsonDocument doc( Config::JSON_CONFIG_SIZE );
  DeserializationError rc = deserializeJson( doc, data );
  // If the JSON config cannot be decoded.
  if( rc != DeserializationError::Ok ) {
    return {RC_ERROR, Utils::format( Messages::JSON_DECODE_ERROR, rc.c_str() )};
  }
  // Build the data from JSON.
  JsonArray array = doc.as<JsonArray>();
  uint8_t i = 0;
  for( JsonObject item : array ) {
    relays[i].alias      = item["alias"].as<char*>();
    relays[i].pin        = item["pin"].as<int>();
    relays[i].inverse    = item["invert"].as<bool>();
    relays[i].persistent = item["persist"].as<bool>();
    if( ++i >= Config::RELAY_MAX_RELAYS ) break;
  }
  // Fill the rest of RelayInfo data if JSON config is too short.
  while( i < Config::RELAY_MAX_RELAYS ) {
    relays[i].alias = "";
    relays[i].pin = 0;
    relays[i].inverse = false;
    relays[i].persistent = false;
    i += 1;
  }
  return RESULT_OK;
}

String RelaysModule::getDefaultRelayData() {
  StaticJsonDocument<Config::JSON_MESSAGE_SIZE * 2> doc;
  JsonArray array = doc.to<JsonArray>();
  for( RelayConfig item : DEFAULT_CONFIG ) {
    JsonObject nested = array.createNestedObject();
    nested["pin"]    = item.pin;
    nested["invert"] = item.inverse;
  }
  return doc.as<String>();
}

String RelaysModule::getRelayData() {
  const String data = getStringOption( "Data" );
  return data.length() > 0 ? data : getDefaultRelayData();
}

String RelaysModule::getRelayName( const uint8_t index ) {
  if( relays[index].alias.length() > 0 ) {
    return relays[index].alias;
  } else {
    String s = "rel";
    s += index + 1;
    return s;
  }
}

void RelaysModule::initializeHardware() {
  for( uint8_t i = 0; i < Config::RELAY_MAX_RELAYS; i++ ) {
    const RelayInfo info = relays[i];
    if( info.pin == 0 ) continue;

    pinMode( info.pin, OUTPUT );
    if( info.persistent ) {
      const uint16_t values = getShortOption( "Values" );
      uint8_t v = (values & (1 << i)) != 0;
      switchRelay( i, info, v );
    } else {
      switchRelay( i, info, 0 );
    }
  }
}

void RelaysModule::saveRelayValue( const uint8_t index, const RelayInfo info, const uint8_t value ) {
  const String moduleId = getId();
  uint16_t values = getShortOption( "Values" );
  if( value == HIGH ) {
    values |= 1 << index;
  } else {
    values &= ~(1 << index);
  }
  Options::setShort( moduleId, "Values", values );
}

void RelaysModule::switchRelay( const uint8_t index, const RelayInfo info, const uint8_t value ) {
  if( info.pin == 0 ) return;
  switch( value ) {
    // Toggle relay
    case 2: {
      uint8_t v = digitalRead( info.pin ) == LOW ? HIGH : LOW;
      digitalWrite( info.pin, digitalRead( info.pin ) == LOW ? HIGH : LOW );
      if( info.persistent ) saveRelayValue( index, info, v );
      break;
    }
    // Switch on
    case 1:
      digitalWrite( info.pin, info.inverse ? LOW : HIGH );
      if( info.persistent ) saveRelayValue( index, info, HIGH );
      break;
    // Switch off
    case 0:
      digitalWrite( info.pin, info.inverse ? HIGH : LOW );
      if( info.persistent ) saveRelayValue( index, info, LOW );
      break;
    // Invalid value
    default:
      break;
  }
}

uint8_t RelaysModule::toRelayIndex( const String& value ) {
  SWITCH( value.c_str() ) {
    CASE( "rel1" ):   return 0;
    CASE( "rel2" ):   return 1;
    CASE( "rel3" ):   return 2;
    CASE( "rel4" ):   return 3;
    CASE( "rel5" ):   return 4;
    CASE( "rel6" ):   return 5;
    CASE( "rel7" ):   return 6;
    CASE( "rel8" ):   return 7;

    DEFAULT_CASE: {
      // Check relays aliases.
      for( uint8_t i = 0; i < Config::RELAY_MAX_RELAYS; i++ ) {
        if( relays[i].alias == value ) return i;
      }
      // Invalid value.
      return 255;
    }
  }
}

/* Private static */

String RelaysModule::getRelayHtmlStatus( const RelayInfo info ) {
  if( info.pin == 0 )  return "btn-outline-dark disabled";

  uint8_t v = digitalRead( info.pin );
  if( info.inverse ) v ^= 1;
  return v == HIGH ? "btn-primary" : "btn-secondary";
}

String RelaysModule::getRelayId( const uint8_t index ) {
  if( index >= 0 && index < Config::RELAY_MAX_RELAYS ) {
    String s = "rel";
    s += index + 1;
    return s;
  } else {
    return "unknown";
  }
}

uint8_t RelaysModule::parseRelayPayload( const String& payload ) {
  SWITCH( payload.c_str() ) {
    CASE( "0" ):
    CASE( "off" ):
      return 0;

    CASE( "1" ):
    CASE( "on" ):
      return 1;

    CASE( "2" ):
    CASE( "toggle" ):
      return 2;

    DEFAULT_CASE:
      return 255;
  }
}

String RelaysModule::toJsonString( const uint8_t index, const RelayInfo info ) {
  StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
  json["id"] = getRelayId( index );
  if( info.alias.length() > 0 ) {
    json["alias"] = info.alias;
  }
  if( info.pin > 0 ) {
    uint8_t value = digitalRead( info.pin );
    if( info.inverse ) value = !value;
    json["state"] = value > 0 ? "ON" : "OFF";
  } else {
    json["state"] = "No pin";
  }
  return json.as<String>();
}