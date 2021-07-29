#include "AM312Module.h"
#include "Events.h"
#include "str_switch.h"
#include "Utils.h"

AM312Module::AM312Module() {
  properties.has_module_webpage = true;
  properties.has_status_webpage = true;
  properties.tick_100mS_required = true;

  sensorPin = getByteOption( PIN_OPTION_KEY, Config::AM312_PIN );
  holdValueMs = getShortOption( HOLD_OPTION_KEY ) * 1000;
  pinMode( sensorPin, INPUT );
}

AM312Module::~AM312Module() {
  // do nothing
}

void AM312Module::tick_100mS( uint8_t phase ) {
  if( phase == 2 ) {
    const bool v = digitalRead( sensorPin );
    const bool sensorChanged = v != sensorValue;
    const bool valueOnHold = statusValue && !v;

    if( sensorChanged || valueOnHold ) {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["motion"] = v;
      const String payload = json.as<String>();

      if( sensorChanged ) {
        sensorValue = v;
        // Send the status changed event to the EventBus.
        // The new state (in JSON format) is provided as the event payload.
        Bus.notify<StatusChangedEvent>( {this, payload} );

        if( v ) {
          lastMotionTimestamp = millis();
          if( !statusValue ) {
            statusValue = true;
            Bus.notify<CommandResponseEvent>( {this, getId(), payload} );
          }
        } else if( holdValueMs ) {
          lastMotionTimestamp = millis();
        }
      }
      // Check an expiration time of pending motion on hold state.
      if( valueOnHold ) {
        const unsigned long delta = millis() - lastMotionTimestamp;
        if( delta > holdValueMs ) {
          statusValue = false;
          Bus.notify<CommandResponseEvent>( {this, getId(), payload} );
        }
      }
    }
  }
}

/* Module Web interface */

const String AM312Module::getModuleWebpage() {
  return makeWebpage( "/module_am312.html" );
}

const String AM312Module::getStatusWebpage() {
  return makeWebpage( "/status_am312.html" );
}

/* A generic getData/setData interface */

const String AM312Module::getString( const String& key ) {
  SWITCH( key.c_str() ) {
    // Get the module export config (JSON string)
    CASE( Config::KEY_EXPORT_CONFIGURATION ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["pin"] = getByteOption( PIN_OPTION_KEY );
      json["hold"] = getShortOption( HOLD_OPTION_KEY );
      return json.as<String>();
    }
    DEFAULT_CASE:
      return Module::getString( key );
  }
}

ResultData AM312Module::setString( const String& key, const String& value ) {
  SWITCH( key.c_str() ) {
    // Permanently store the new module config.
    CASE( Config::KEY_IMPORT_CONFIGURATION ):
      return handleConfigImport( value );
    DEFAULT_CASE:
      return Module::setString( key, value );
  }
}

/* Protected */

ResultData AM312Module::handleOption( const String& key, const String& value, Options::Action action ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    CASE( "pin" ): {
      if( action == Options::VERIFY ) {
        uint8_t pin = Utils::toByte( value.c_str() );
        if( pin < 1 || pin > 39 ) return INVALID_VALUE;
      }
      return handleByteOption( PIN_OPTION_KEY, value, action, false );
    }
    // ==========================================
    CASE( "hold" ): {
      ResultData rc = handleShortOption( HOLD_OPTION_KEY, value, action, false );
      if( action == Options::SAVE && rc.code == RC_OK ) {
        uint16_t v = atoi( value.c_str() );
        holdValueMs = v * 1000;
      }
      return rc;
    }
    // ==========================================
    DEFAULT_CASE:
      return UNKNOWN_OPTION;
  }
}

void AM312Module::resolveTemplateKey( const String& key, String& out ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    // Module template parameters
    CASE( "TITLE" ):     out += Utils::formatModuleSettingsTitle( getId(), getName() );  break;
    CASE( "PIN" ):       out += getByteOption( PIN_OPTION_KEY, Config::AM312_PIN );      break;
    CASE( "HOLD" ):      out += getShortOption( HOLD_OPTION_KEY );                       break;
    // ==========================================
    // Status template parameters
    CASE( "MOTION" ):    out += Utils::format( Messages::AM_312_MOTION, Utils::toString( sensorValue ));  break;
  }
}
