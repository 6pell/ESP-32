#include "BH1750Module.h"
#include "Events.h"
#include "str_switch.h"
#include "Utils.h"

BH1750Module::BH1750Module() {
  properties.has_module_webpage = true;
  properties.has_status_webpage = true;
  properties.tick_100mS_required = true;

  pollIntervalMs = getPollOption() * 1000;
  valueDelta = getValueDeltaOption().toFloat();

  Wire.begin();
  if( !sensor.begin( BH1750::ONE_TIME_LOW_RES_MODE )) {
    Log.error( "BH1750 sensor did not respond" CR );
  } else {
    sensorDetected = true;
  }
}

BH1750Module::~BH1750Module() {
  // do nothing
}

void BH1750Module::tick_100mS( uint8_t phase ) {
  if( !sensorDetected ) return;

  // Read the sensor every xx seconds
  if( pollIntervalMs > 0 ) {
    const unsigned long now = millis();
    const unsigned long delta = now - lastMeasureTimestamp;
    if( delta > pollIntervalMs ) {
      lastMeasureTimestamp = now;
      const float newValue = sensor.readLightLevel();
      if( previousValue != newValue ) {
        lux = String( newValue );
        // Send the status changed event to the EventBus.
        // The new state (in a JSON format) is provided as the event payload.
        StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
        json["lux"] = lux;
        const String payload = json.as<String>();
        Bus.notify( (StatusChangedEvent) {this, payload} );
        // Send an event if value is changed more than dalta threshold option.
        if( std::abs(previousValue - newValue) > valueDelta ) {
          previousValue = newValue;
          Bus.notify<CommandResponseEvent>( (CommandResponseEvent) {this, getId(), payload} );
        }
      }
    }
  }
}

/* Module Web interface */

const String BH1750Module::getModuleWebpage() {
  return makeWebpage( "/module_bh1750.html" );
}

const String BH1750Module::getStatusWebpage() {
  return makeWebpage( "/status_bh1750.html" );
}

/* A generic getData/setData interface */

const String BH1750Module::getString( const String& key ) {
  SWITCH( key.c_str() ) {
    // Get the module export config (JSON string)
    CASE( Config::KEY_EXPORT_CONFIGURATION ): {
    StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
    json["poll"] = getPollOption();
    json["delta"] = getValueDeltaOption();
    return json.as<String>();
    }
    DEFAULT_CASE:
      return Module::getString( key );
  }
}

ResultData BH1750Module::setString( const String& key, const String& value ) {
  SWITCH( key.c_str() ) {
    // Permanently store the new module config.
    CASE( Config::KEY_IMPORT_CONFIGURATION ):
      return handleConfigImport( value );
    DEFAULT_CASE:
      return Module::setString( key, value );
  }
}

/* Protected */

ResultData BH1750Module::handleOption( const String& key, const String& value, Options::Action action ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    CASE( "poll" ): {
      ResultData rc = handleShortOption( POLL_OPTION_KEY, value, action, false );
      if( action == Options::SAVE && rc.code == RC_OK ) {
        uint16_t interval = atoi( value.c_str() );
        pollIntervalMs = interval * 1000;
      }
      return rc;
    }
    // ==========================================
    CASE( "delta" ):
      if( action != Options::READ ) {
        if( !Utils::isNumber( value.c_str()) )  return INVALID_VALUE;
        if( action == Options::SAVE ) {
          valueDelta = value.toFloat();
          setStringOption( DELTA_OPTION_KEY, value );
        }
      }
      return {RC_OK, getValueDeltaOption()};
    // ==========================================
    DEFAULT_CASE:
      return UNKNOWN_OPTION;
  }
}

void BH1750Module::resolveTemplateKey( const String& key, String& out ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    // Module template parameters
    CASE( "POLLTIME" ):  out += getPollOption();                              break;
    CASE( "DELTA" ):     out += getValueDeltaOption();                        break;
    // ==========================================
    // Status template parameters
    CASE( "TITLE" ):    out += Messages::TITLE_BH1750_MODULE;                break;
    CASE( "LUX" ):      out += Utils::format( Messages::BME1750_LUX, lux );  break;
  }
}

/* Private */

uint16_t BH1750Module::getPollOption() {
  return getShortOption( POLL_OPTION_KEY, Config::BH1750_POLL_INTERVAL );
}

String BH1750Module::getValueDeltaOption() {
  return getStringOption( DELTA_OPTION_KEY, String(Config::BH1750_LUX_DELTA) );
}
