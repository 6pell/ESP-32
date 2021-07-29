#include <Wire.h>
#include "BME280Module.h"
#include "Events.h"
#include "str_switch.h"
#include "Utils.h"

BME280Module::BME280Module() {
  properties.has_module_webpage = true;
  properties.has_status_webpage = true;
  properties.tick_100mS_required = true;

  state = SLEEP;
  pollIntervalMs = getPollOption() * 1000;
  temperatureDelta = getDeltaTOption().toFloat();
  humidityDelta = getDeltaHOption().toFloat();

  Wire.begin();
  sensor.setI2CAddress( Config::BME280_SENSOR_ADDR );

  if( !sensor.beginI2C() ) {
    Log.error( "BME280 sensor did not respond" CR );
  } else {
    sensorDetected = true;
    sensor.setMode( MODE_SLEEP );   // Sleep for now
  }
}

BME280Module::~BME280Module() {
  // do nothing
}

void BME280Module::tick_100mS( uint8_t phase ) {
  if( !sensorDetected ) return;

  switch( state ) {
    case WAKEUP:
      if( !sensor.isMeasuring() ) {
        // Read values from the sensor
        const float temp = getScaleOption() == Config::CELSIUS ? sensor.readTempC() : sensor.readTempF();
        const float hum = sensor.readFloatHumidity();
        temperature = String( temp );
        humidity = String( hum );
        // Send the status changed event to the EventBus.
        // The new state (in a JSON format) is provided as the event payload.
        const String json = toJsonString();
        Bus.notify( (StatusChangedEvent) {this, json} );
        // Send a packet of new values to the EventBus.
        if( std::abs(previousTemperature - temp) > temperatureDelta || std::abs(previousHumidity - hum) > humidityDelta ) {
          previousTemperature = temp;
          previousHumidity = hum;
          Bus.notify<CommandResponseEvent>( (CommandResponseEvent) {this, getId(), json} );
        }
        state = SLEEP;
      }
      break;

    case SLEEP: {
      if( pollIntervalMs > 0 ) {
        const unsigned long now = millis();
        const unsigned long delta = now - lastMeasureTimestamp;
        if( delta > pollIntervalMs ) {
          lastMeasureTimestamp = now;
          // Wake up sensor and take reading
          sensor.setMode( MODE_FORCED );
          state = WAKEUP;
        }
      }
      break;
    }
  }
}

/* Module Web interface */

const String BME280Module::getModuleWebpage() {
  return makeWebpage( "/module_bme280.html" );
}

const String BME280Module::getStatusWebpage() {
  return makeWebpage( "/status_bme280.html" );
}

/* A generic getData/setData interface */

const String BME280Module::getString( const String& key ) {
  SWITCH( key.c_str() ) {
    // Get the module export config (JSON string)
    CASE( Config::KEY_EXPORT_CONFIGURATION ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["scale"]  = toString( getScaleOption() );
      json["poll"]   = getPollOption();
      json["deltaT"] = getDeltaTOption();
      json["deltaH"] = getDeltaHOption();
      return json.as<String>();
    }
    DEFAULT_CASE:
      return Module::getString( key );
  }
}

ResultData BME280Module::setString( const String& key, const String& value ) {
  SWITCH( key.c_str() ) {
    // Permanently store the new relay config (JSON array string).
    CASE( Config::KEY_IMPORT_CONFIGURATION ):
      return handleConfigImport( value );
    DEFAULT_CASE:
      return Module::setString( key, value );
  }
}

/* Protected */

bool BME280Module::handleCommand( const String& cmd, const String& args ) {
  return false;
}

ResultData BME280Module::handleOption( const String& key, const String& value, Options::Action action ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    CASE( "scale" ): {
      auto scale = toScale( value );
      if( scale == Config::UNKNOWN_SCALE && action != Options::READ ) {
        return INVALID_VALUE;
      }
      ResultData rc = handleByteOption( "Units", String(scale), action, false );
      rc.details = toString( getScaleOption() );
      return rc;
    }
    // ==========================================
    CASE( "poll" ): {
      ResultData rc = handleShortOption( "Poll", value, action, false );
      if( action == Options::SAVE && rc.code == RC_OK ) {
        uint16_t interval = atoi( value.c_str() );
        pollIntervalMs = interval * 1000;
      }
      return rc;
    }
    // ==========================================
    CASE( "deltaT" ):
      if( action != Options::READ ) {
        if( !Utils::isNumber( value.c_str()) )  return INVALID_VALUE;
        if( action == Options::SAVE ) {
          temperatureDelta = value.toFloat();
          setStringOption( "DeltaT", value );
        }
      }
      return {RC_OK, getDeltaTOption()};
    // ==========================================
    CASE( "deltaH" ):
      if( action != Options::READ ) {
        if( !Utils::isNumber( value.c_str()) )  return INVALID_VALUE;
        if( action == Options::SAVE ) {
          humidityDelta = value.toFloat();
          setStringOption( "DeltaH", value );
        }
      }
      return {RC_OK, getDeltaHOption()};
    // ==========================================
    DEFAULT_CASE:
      return UNKNOWN_OPTION;
  }
}

void BME280Module::resolveTemplateKey( const String& key, String& out ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    // Module template parameters
    CASE( "MTITLE" ):    out += Utils::formatModuleSettingsTitle( getId(), getName() );  break;
    CASE( "POLLTIME" ):  out += getPollOption();                                         break;
    CASE( "DELTA_T" ):   out += getDeltaTOption();                                       break;
    CASE( "DELTA_H" ):   out += getDeltaHOption();                                       break;
    CASE( "SCALE_C" ):   out += getScaleOption() == Config::CELSIUS ? "selected" : "";   break;
    CASE( "SCALE_F" ):   out += getScaleOption() == Config::FARENHEIT ? "selected" : ""; break;
    // ==========================================
    // Status template parameters
    CASE( "STITLE" ):    out += Messages::TITLE_BME280_MODULE;                           break;
    CASE( "SHUM" ):      out += Utils::format( Messages::BME280_HUMIDITY, humidity );    break;
    CASE( "STEMP" ): {
      out += Utils::format( Messages::BME280_TEMPERATURE, temperature, getScaleOption() == Config::CELSIUS ? "C" : "F" );
      break;
    }
  }
}

/* Private */

String BME280Module::getDeltaTOption() {
  return getStringOption( "DeltaT", String(Config::BME280_TEMPERATURE_DELTA) );
}

String BME280Module::getDeltaHOption() {
  return getStringOption( "DeltaH", String(Config::BME280_HUMIDITY_DELTA) );
}

uint16_t BME280Module::getPollOption() {
  return getShortOption( "Poll", Config::BME280_POLL_INTERVAL );
}

Config::TemperatureScale BME280Module::getScaleOption() {
  return static_cast<Config::TemperatureScale>(getByteOption( "Scale", Config::BME280_TEMPERATURE_SCALE ));
}

String BME280Module::toJsonString() {
  StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
  json["temperature"] = temperature;
  json["humidity"] = humidity;
  return json.as<String>();
}

Config::TemperatureScale BME280Module::toScale( const String& value ) {
  if( value == "C" || value == "celsius" )  return Config::CELSIUS;
  if( value == "F" || value == "farenheit" )  return Config::FARENHEIT;
  return Config::UNKNOWN_SCALE;
}

String BME280Module::toString( const Config::TemperatureScale scale ) {
  switch( scale ) {
    case Config::CELSIUS:   return "celsius";
    case Config::FARENHEIT: return "farenheit";
    default:                return "unknown";
  }
}