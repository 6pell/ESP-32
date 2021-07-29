#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include "str_switch.h"
#include "Utils.h"
#include "backlight/BackLightModule.h"
#include "backlight/Factory.h"
#include "backlight/RgbPalette.h"

BackLightModule::BackLightModule() {
  properties.has_module_webpage = true;

  StaticJsonDocument<Config::JSON_MESSAGE_SIZE> doc;
  DeserializationError rc = deserializeJson( doc, getJsonConfig() );
  // Instantiate the strip object with JSON configuration.
  if( rc == DeserializationError::Ok && doc.size() > 0 ) {
    JsonObject json = doc.as<JsonObject>();
    const uint8_t pin = json[PIN_OPTION_KEY];
    const uint16_t pixelsCount = json[PIXELS_OPTION_KEY];
    strip = new NeoPixelWrapper( pixelsCount, pin );
    strip->setMaxPowerBudget( getMaxPowerBudget( json ));
    fixWhiteColor = getFixWhiteOption( json );
  }
  // Instantiate the strip object with default parameters.
  else {
    const uint8_t pin = Config::BACKLIGHT_PIN;
    const uint16_t pixelsCount = Config::BACKLIGHT_PIXELS_COUNT;
    strip = new NeoPixelWrapper( pixelsCount, pin );
    strip->setMaxPowerBudget( Config::BACKLIGHT_MAX_POWER_BUDGET );
    fixWhiteColor = RgbColor( HtmlColor( 0xFFFFFF ));
    // Store the default module configuration.
    doc.clear();
    doc[PIN_OPTION_KEY] = pin;
    doc[PIXELS_OPTION_KEY] = pixelsCount;
    doc[MAX_POWER_OPTION_KEY] = Config::BACKLIGHT_MAX_POWER_BUDGET;
    doc[FIX_WHITE_OPTION_KEY] = "0xFFFFFF";
    setJsonConfig( doc.as<String>() );
  }

  // Final setup steps.
  strip->begin();
  reset( true );
}

BackLightModule::~BackLightModule() {
  reset( true );
  delete strip;
}

/* Public virtual */

const String BackLightModule::getModuleWebpage() {
  return makeWebpage( "/module_backlight.html" );
}

const String BackLightModule::getString( const String& key ) {
  SWITCH( key.c_str() ) {
    CASE( Config::KEY_EXPORT_CONFIGURATION ): {
      DynamicJsonDocument doc( Config::JSON_CONFIG_SIZE );
      // Export the module config.
      doc["config"] = serialized( getJsonConfig() );
      // Export the configuration of all effects.
      // Entry with index 0 is a "Select an effect" one, it's ignored.
      const uint8_t count = Backlight::Factory::getEffectsCount();
      for( uint8_t i = 1; i < count; i++ ) {
        const String id = Backlight::Factory::getEffectId( i );
        const String options = Options::getString( "fx", id );
        if( !options.isEmpty() ) {
          doc[id] = serialized( options );
        }
      }
      return doc.as<String>();
    }
    DEFAULT_CASE: {
      // Serve the module webpage request to obtain effect options.
      if( Backlight::Factory::findEffectById( key ) > 0 ) {
        const String options = Options::getString( "fx", key );
        return options.isEmpty()
          ? makeDefaultEffectOptions( key )
          : options;
      }
      // Unknown key, so return an empty string.
      else {
        return "";
      }
    }
  }
}

ResultData BackLightModule::setString( const String& key, const String& value ) {
  SWITCH( key.c_str() ) {
    CASE( Config::KEY_IMPORT_CONFIGURATION ):
      //TODO
      //return handleConfigImport( value );
    DEFAULT_CASE:
      return Module::setString( key, value );
  }
}

/* Protected */

bool BackLightModule::handleCommand( const String& cmd, const String& args ) {
  SWITCH( cmd.c_str() ) {
    // ==========================================
    // Adjust the effect parameters.
    // payload: JSON object with effect parameters.
    CASE( "adjust" ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> doc;
      DeserializationError rc = deserializeJson( doc, args );

      if( rc != DeserializationError::Ok ) {
        handleCommandResults( cmd, args, rc.c_str() );
        return true;
      }

      const JsonObject json = doc.as<JsonObject>();
      if( !json.containsKey( Backlight::EFFECT_ID_KEY )) {
        handleCommandResults( cmd, args, Messages::EFFECT_ID_MISSED );
        return true;
      }

      const String id = json[Backlight::EFFECT_ID_KEY];
      reset( lastEffectId != id );
      lastEffectId = id;
      effect = Backlight::Factory::createEffect( id, strip, animations );

      if( effect ) {
        performEffect( args );
        handleCommandResults( cmd, args, Messages::OK );
      } else {
        reset( true );
        handleCommandResults( cmd, args, Messages::EFFECT_CREATE_FAILED );
      }
      return true;
    }
    // ==========================================
    // Turn on the strip.
    // payload: the effect ID.
    CASE( "on" ): {
      // Create an effect.
      reset( false );
      effect = Backlight::Factory::createEffect( args, strip, animations );
      if( !effect ) {
        handleCommandResults( cmd, args, Messages::EFFECT_CREATE_FAILED );
      } else {
        performEffect( Options::getString( "fx", args ));
        handleCommandResults( cmd, args, Messages::OK );
      }
      return true;
    }
    // ==========================================
    // Turn off the strip.
    CASE( "off" ):
      reset( true );
      handleCommandResults( cmd, args, Messages::OK );
      return true;
    // ==========================================
    CASE( "power" ):
      handleCommandResults( cmd, args, String( strip->getStripCurrent() ));
      return true;
    // ==========================================
    DEFAULT_CASE:
      return false;
  }
}

ResultData BackLightModule::handleOption( const String& key, const String& value, Options::Action action ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    // Save the new module config.
    CASE( "config" ): {
      if( action != Options::READ ) {
        StaticJsonDocument<Config::JSON_CONFIG_SIZE> doc;
        const DeserializationError rc = deserializeJson( doc, value );
        // validation
        if( action == Options::VERIFY ) {
          if( rc != DeserializationError::Ok ) {
            return {RC_INVALID_VALUE, rc.c_str()};
          }
        }
        // save
        else if( action == Options::SAVE ) {
          const JsonObject json = doc.as<JsonObject>();
          // Read parameters.
          fixWhiteColor = getFixWhiteOption( json );
          strip->setMaxPowerBudget( getMaxPowerBudget( json ));
          // Recreate the strip object with new received parameters.
          const uint8_t pin = json[PIN_OPTION_KEY];
          const uint16_t pixelsCount = json[PIXELS_OPTION_KEY];
          if( strip->getPin() != pin || strip->getPixelsCount() != pixelsCount ) {
            reset( true );
            delete strip;
            strip = new NeoPixelWrapper( pixelsCount, pin );
            strip->begin();
          }
          // Save the new config.
          setJsonConfig( value );
        }
      }
      return {action == Options::SAVE ? RC_OK_REINIT : RC_OK, value};
    }
    // ==========================================
    // Decode the effect JSON, store effect options.
    CASE( "effect" ): {
      if( action != Options::READ ) {
        StaticJsonDocument<Config::JSON_MESSAGE_SIZE> doc;
        const DeserializationError rc = deserializeJson( doc, value );
        // validation
        if( action == Options::VERIFY ) {
          if( rc != DeserializationError::Ok ) {
            return {RC_INVALID_VALUE, rc.c_str()};
          }
          if( !doc.containsKey( Backlight::EFFECT_ID_KEY )) {
            return {RC_INVALID_VALUE, Messages::EFFECT_ID_MISSED};
          }
          const String id = doc[Backlight::EFFECT_ID_KEY];
          if( Backlight::Factory::findEffectById( id ) == -1 ) {
            return {RC_ERROR, Messages::EFFECT_CREATE_FAILED};
          }
        }
        // save
        else if( action == Options::SAVE ) {
          String id = doc[Backlight::EFFECT_ID_KEY];
          StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
          json[Backlight::BRIGHTNESS_KEY] = doc[Backlight::BRIGHTNESS_KEY];
          json[Backlight::COLOR_KEY]      = doc[Backlight::COLOR_KEY];
          json[Backlight::PALETTE_KEY]    = doc[Backlight::PALETTE_KEY];
          json[Backlight::INTENSITY_KEY]  = doc[Backlight::INTENSITY_KEY];
          json[Backlight::SPEED_KEY]      = doc[Backlight::SPEED_KEY];
          Options::setString( "fx", id, json.as<String>() );
        }
      }
      return {RC_OK, value};
    }
    // ==========================================
    // If the option's key matches the effect ID, consider it as a 'save' command.
    DEFAULT_CASE: {
      if( Backlight::Factory::findEffectById( key ) > 0 ) {
        if( action == Options::SAVE ) {
          Options::setString( "fx", key, value );
        }
        return {RC_OK, value};
      }
      return UNKNOWN_OPTION;
    }
  }
}

void BackLightModule::resolveTemplateKey( const String& key, String& out ) {
  SWITCH( key.c_str() ) {
    CASE( "FxId" ): {
      out += buildEffectsHtmlOptions();
      break;
    }
    CASE( "FxPalette" ): {
      out += buildPalettesHtmlOptions();
      break;
    }
    CASE( "Config" ): {
      out += getJsonConfig();
      break;
    }
    CASE( "Title" ): {
      out += Utils::formatModuleSettingsTitle( getId(), getName() );
      break;
    }
  }
}

/* Private */

void BackLightModule::performEffect( const String& jsonOptions ) {
  // Apply options.
  effect->setOptions( jsonOptions );
  if( effect->getColor() == RgbColor( 255,255,255 )) {
    effect->setColor( fixWhiteColor );
  }
  if( effect)
  // Perform an effect.
  effect->perform();
}

void BackLightModule::reset( bool resetStrip ) {
  animations.StopAll();

  if( resetStrip ) {
    strip->clearTo( RgbColor( 0 ));
    strip->show();
  }

  if( effect ) {
    delete effect;
    effect = nullptr;
  }
}

String BackLightModule::buildEffectsHtmlOptions() {
  String out;
  uint8_t count = Backlight::Factory::getEffectsCount();
  for( uint8_t i = 0; i < count; i++ ) {
    const String id = Backlight::Factory::getEffectId( i );
    const String title = Backlight::Factory::getEffectTitle( i );
    appendOption( out, id, title );
  }
  return out;
}

String BackLightModule::buildPalettesHtmlOptions() {
  String out;
  uint8_t count = Backlight::RgbPalette16::getPalettesCount();
  for( uint8_t i = 0; i < count; i++ ) {
    const String id = Backlight::RgbPalette16::getPaletteId( i );
    const String title = Backlight::RgbPalette16::getPaletteTitle( i );
    appendOption( out, id, title );
  }
  return out;
}

String BackLightModule::getJsonConfig() {
  return getStringOption( MODULE_CONFIG_KEY );
}

void BackLightModule::setJsonConfig( const String& cfg ) {
  setStringOption( MODULE_CONFIG_KEY, cfg );
}

/* Private static */

void BackLightModule::appendOption( String& out, const String& id, const String& title ) {
  char buf[64];
  snprintf( buf, sizeof( buf ), "<option value=\"%s\">%s</option>", id.c_str(), title.c_str() );
  out += buf;
}

RgbColor BackLightModule::getFixWhiteOption( const JsonObject& json ) {
  if( json.containsKey( FIX_WHITE_OPTION_KEY )) {
    const char* value = json[FIX_WHITE_OPTION_KEY];
    uint32_t v = strtoul( value, NULL, 16 );
    if( v == 0 ) v = 0xFFFFFF;
    return RgbColor( HtmlColor( v ));
  } else {
    return RgbColor( HtmlColor( 0xFFFFFF ));
  }
}

uint16_t BackLightModule::getMaxPowerBudget( const JsonObject& json ) {
  if( json.containsKey( MAX_POWER_OPTION_KEY )) {
    uint16_t v = json[MAX_POWER_OPTION_KEY];
    return v;
  } else {
    return Config::BACKLIGHT_MAX_POWER_BUDGET;
  }
}

String BackLightModule::makeDefaultEffectOptions( const String& _id ) {
  StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
  json[Backlight::EFFECT_ID_KEY]  = _id;
  json[Backlight::BRIGHTNESS_KEY] = Backlight::DEFAULT_BRIGHTNESS;
  json[Backlight::COLOR_KEY]      = "#FFFFFF";
  json[Backlight::PALETTE_KEY]    = "Default";
  json[Backlight::SPEED_KEY]      = Backlight::DEFAULT_SPEED;
  json[Backlight::INTENSITY_KEY]  = Backlight::DEFAULT_INTENSITY;
  return json.as<String>();
}
