#include <ArduinoLog.h>
#include <SPIFFS.h>
#include "Messages.h"
#include "Events.h"
#include "Module.h"
#include "str_switch.h"
#include "Utils.h"

const String Module::getModuleWebpage() {
  return makeWebpage( "/module_project.html" );
}

const String Module::getStatusWebpage() {
  return makeWebpage( "/module_project.html" );
}

// A generic getData/setData interface

const uint8_t Module::getByte( const String& key ) {
  SWITCH( key.c_str() ) {
    // Get the module card position on the status web interface page.
    CASE( "StatusPos" ):
      return getByteOption( key );
    DEFAULT_CASE:
      return 0;
  }
}

void Module::setByte( const String& key, const uint8_t value ) {
  SWITCH( key.c_str() ) {
    // Set the module card position on the status web interface page.
    CASE( "StatusPos" ):
      setByteOption( key, value );
      break;
  }
}

ResultData Module::setString( const String& key, const String& value ) {
  SWITCH( key.c_str() ) {
    CASE( Config::KEY_IMPORT_CONFIGURATION ):
      return RESULT_OK;
    DEFAULT_CASE:
      return UNKNOWN_COMMAND;
  }
}

/**
 * Try to execute the command on module using the virtual handleCommand() method.
 * @return true if command was handled, no matters successfully or not, and no other actions are needed.
 *         false if the handleCommand() method wasn't processed it.
 */
bool Module::dispatchCommand( const String& command ) {
  auto pair = Utils::split( command );
  bool handled = handleCommand( pair.first, pair.second );
  if( !handled ) {
    // Try to handle the command as module settings update using the virtual handleOption() method.
    // The '' is a special case meaning an empty string.
    auto action = pair.second.length() > 0 ? Options::SAVE : Options::READ;
    if( pair.second == "''" )  pair.second = "";
    ResultData result = handleOption( pair.first, pair.second, action );
    handleCommandResults( pair.first, pair.second, result.details );
    handled = result.code == RC_OK;
  }
  return handled;
}

ResultData Module::dispatchSettings( const std::map<String,String>& map ) {
  // 1st step: only validate options in the map, one by one.
  for( auto const& x : map ) {
    // The module_id is a module identifier, not an option key, so should be skipped.
    if( x.first == "module_id" ) continue;
    ResultData result = handleOption( x.first, x.second, Options::VERIFY );
    // Intercept the "unknown option" error
    if( result.code == RC_UNKNOWN_OPTION ) {
      String msg = Messages::SETTINGS_UNKNOWN_OPTION;
      msg += " ";
      msg += x.first;
      return {RC_UNKNOWN_OPTION, msg};
    }
    // Intercept the "invalid value" error
    else if( result.code == RC_INVALID_VALUE ) {
      return {RC_INVALID_VALUE, x.first + Messages::SETTINGS_INVALID_VALUE};
    }
  }
  // 2nd step: store options.
  RetCode finalRC = RC_UNCHANGED;
  for( auto const& x : map ) {
    // The module_id is a module identifier, not an option key, so should be skipped.
    if( x.first == "module_id" ) continue;
    // Store options in the map, one by one.
    ResultData result = handleOption( x.first, x.second, Options::SAVE );
    switch( result.code ) {
      // Restart has the highest prio in all 'ok' return codes and cannot be overwritten.
      case RC_OK_RESTART:
        finalRC = RC_OK_RESTART;
        break;
      // Reinit status code can be overwritten only by restart one.
      case RC_OK_REINIT:
        if( finalRC != RC_OK_RESTART ) {
          finalRC = RC_OK_REINIT;
        }
        break;
      // OK status code.
      case RC_OK:
        if( finalRC != RC_OK_RESTART || finalRC != RC_OK_REINIT ) {
          finalRC = RC_OK;
        }
        break;
      // All other codes indicates an error. Return an error immediately.
      default:
        return result;
    }
  }
  return {finalRC, Messages::SETTINGS_SAVED_OK};
}

/* Protected */

void Module::handleCommandResults( const String& cmd, const String& args, const String& result ) {
  // Send exec results to EventBus.
  const String out = Utils::toResultsJson( cmd, args, result );
  Bus.notify<CommandResponseEvent>( (CommandResponseEvent) {this, getId(), out} );
}

/**
 * Options of string type may have macro identifiers, that are begins with a '#' char.
 * This method resolves known macro parameters, i.e. substitutes macros into values.
 */
const String Module::getMacroOptionOf( const String& moduleId, const String& optionKey, const String& defValue ) {
  const String key = Options::makeKey( moduleId, optionKey );
  String option = Options::preferences.getString( key.c_str(), defValue );
  // MQTT topic
  if( option.indexOf( "#TOPIC" ) != -1 ) {
    const String topic_key = Options::makeKey( MQTT_MODULE, "Topic" );
    option.replace( "#TOPIC", Options::preferences.getString( topic_key.c_str(), Config::MQTT_DEVICE_TOPIC ));
  }
  // 4 last digits of MAC addres
  if( option.indexOf( "#MAC4" ) != -1 ) {
    uint16_t magic = (ESP.getEfuseMac() >> 32) & 0x1FFF;
    option.replace( "#MAC4", String(magic) );
  }
  return option;
}

ResultData Module::handleConfigImport( const String& data ) {
  StaticJsonDocument<Config::JSON_CONFIG_SIZE> doc;
  DeserializationError rc = deserializeJson( doc, data );
  if( rc != DeserializationError::Ok ) {
    return {RC_ERROR, Utils::format( Messages::JSON_DECODE_ERROR, rc.c_str() )};
  } else {
    JsonObject json = doc.as<JsonObject>();
    for( JsonPair p : json ) {
      const String key = p.key().c_str();
      const String value = p.value().as<String>();
      const ResultData rc = handleOption( key, value, Options::SAVE );
      if( rc.code != RC_OK ) {
        return rc;
      }
    }
    return RESULT_OK;
  }
}

String Module::makeWebpage( const char* fpath ) {
  File file = SPIFFS.open( fpath );
  if( !file || file.isDirectory() ) {
    return String( "Failed to open file " ) + fpath;
  }

  size_t fsize = file.size();
  if( fsize > 0 ) {
    // Read the file into a buffer
    std::unique_ptr<char[]> buffer(new char[fsize]);
    file.readBytes( buffer.get(), fsize );
    file.close();

    // Resolve template parameters.
    String key;
    String out;
    out.reserve( fsize + fsize * 10 / 100 );    // file size + 10%

    bool have_key = false;
    char* content = buffer.get();
    while( fsize-- ) {
      char c = *content++;
      if( have_key ) {
        if( c == '%' ) {
          resolveTemplateKey( key, out );
          have_key = false;
          key = "";
        } else if( key.length() < 9 ) {     // SWITCH macro requires 9 chars max
          key += c;
        }
      } else {
        if( c == '%' ) {
          have_key = true;
        } else {
          out += c;
        }
      }
    }
    return out;
  } else {
    return "";
  }
}

ResultData Module::handleByteOption( const String& key, const String& value, Options::Action action, bool important ) {
  const String moduleId = getId();
  bool modified = false;
  if( action != Options::READ ) {
    if( !Utils::isNumber( value.c_str() )) {
      return INVALID_VALUE;
    }
    if( action == Options::SAVE ) {
      const uint8_t v = value.toInt();
      modified = Options::getByte( moduleId, key ) != v;
      Options::setByte( moduleId, key, v );
    }
  }
  const RetCode rc = (modified && action == Options::SAVE && important) ? RC_OK_REINIT : RC_OK;
  return {rc, String( Options::getByte( moduleId, key ))};
}


ResultData Module::handleIpAddressOption( const String& key, const String& value, Options::Action action, Options::StringConstraints cs ) {
  const String moduleId = getId();
  bool modified = false;                              // true only if old and new values differs
  if( action != Options::READ ) {
    if( value.length() > 0 ) {                        // value isn't empty
      uint32_t addr;
      if( !Utils::parseIpString( value.c_str(), &addr )) {
        return INVALID_VALUE;
      }
      if( action == Options::SAVE ) {
        modified = Options::getLong( moduleId, key ) != addr;
        Options::setLong( moduleId, key, addr );
      }
    }
  } else {                                            // value is empty
    if( cs.notEmpty ) {
      return INVALID_VALUE;
    }
    if( action == Options::SAVE ) {
      modified = Options::getLong( moduleId, key ) != 0;
      Options::setLong( moduleId, key, 0 );
    }
  }
  const RetCode rc = (modified && action == Options::SAVE && cs.important) ? RC_OK_REINIT : RC_OK;
  const String ip = IPAddress( Options::getLong( moduleId, key )).toString();
  return {rc, ip};
}

ResultData Module::handleLongOption( const String& key, const String& value, Options::Action action, bool important ) {
  const String moduleId = getId();
  bool modified = false;
  if( action != Options::READ ) {
    if( !Utils::isNumber( value.c_str() )) {
      return INVALID_VALUE;
    }
    if( action == Options::SAVE ) {
      const uint16_t v = value.toInt();
      modified = Options::getShort( moduleId, key ) != v;
      Options::setShort( moduleId, key, v );
    }
  }
  const RetCode rc = (modified && action == Options::SAVE && important) ? RC_OK_REINIT : RC_OK;
  return {rc, String( Options::getShort( moduleId, key ))};
}

ResultData Module::handleShortOption( const String& key, const String& value, Options::Action action, bool important ) {
  const String moduleId = getId();
  bool modified = false;
  if( action != Options::READ ) {
    if( !Utils::isNumber( value.c_str() )) {
      return INVALID_VALUE;
    }
    if( action == Options::SAVE ) {
      const uint16_t v = value.toInt();
      modified = Options::getShort( moduleId, key ) != v;
      Options::setShort( moduleId, key, v );
    }
  }
  const RetCode rc = (modified && action == Options::SAVE && important) ? RC_OK_REINIT : RC_OK;
  return {rc, String( Options::getShort( moduleId, key ))};
}

ResultData Module::handleStringOption( const String& key, const String& value, Options::Action action, Options::StringConstraints cs ) {
  const String moduleId = getId();
  bool modified = false;                              // true only if old and new values differs
  if( action != Options::READ ) {
    if( value.length() > 0 ) {                        // value isn't empty
      if( action == Options::SAVE ) {
        modified = Options::getString( moduleId, key ) != value;
        Options::setString( moduleId, key, value );
      }
    } else {                                          // value is empty
      if( cs.notEmpty ) {
        return INVALID_VALUE;
      }
      if( action == Options::SAVE ) {
        modified = Options::getString( moduleId, key ).length() > 0;
        Options::setString( moduleId, key, "" );
      }
    }
  }
  const RetCode rc = (modified && action == Options::SAVE && cs.important) ? RC_OK_REINIT : RC_OK;
  return {rc, Options::getString( moduleId, key )};
}
