#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include <nvs.h>
#include <SPIFFS.h>
#include <StreamString.h>
#include <WiFi.h>
#include <Wire.h>
#include "CoreModule.h"
#include "Events.h"
#include "LogManagerModule.h"
#include "ModulesManager.h"
#include "MqttClientModule.h"
#include "Options.h"
#include "RtcTimeModule.h"
#include "str_switch.h"
#include "Utils.h"
#include "core/ConfigImporter.h"
#include "core/FirmwareUploader.h"

CoreModule::CoreModule() {
  properties.has_module_webpage = true;
  properties.tick_100mS_required = true;
  flags.data = 0;

  // Subscribe to event bus connectivity events.
  eventBusToken = Bus.listen<ConnectivityEvent>( [this](const ConnectivityEvent& event ) {
    if( event.type == ConnectivityEvent::TYPE_MQTT && event.connected ) {
      publishMqttConnectionInfo1();
      publishMqttConnectionInfo2();
    }
  });
  // Subscribe to event bus system events.
  Bus.listen<SystemEvent>( eventBusToken, [this](const SystemEvent& event) {
    if( event.type == SystemEvent::TYPE_PENDING_RESTART ) {
      performPendingRestart();
    }
  });

  Log.notice( "CORE Firmware version %s" CR, Utils::getSystemVersionName().c_str() );

  if( !SPIFFS.begin() ) {
    Log.error( "CORE SPIFFS mount failed" CR );
  }
  // If have any extra options (in JSON format), apply them now.
  auto rc = applyExtraOptions( getStringOption( MODULE_OPTIONS_KEY ));
  if( rc.code != RC_OK ) {
    Log.error( "CORE %s" CR, rc.details );
  }
}

CoreModule::~CoreModule() {
  Bus.unlisten<ConnectivityEvent>( eventBusToken );
}

/**
 * A state loop ticker, called 10 times per second.
 */
void CoreModule::tick_100mS( uint8_t phase ) {
  // Periodically send the telemetry info.
  const uint16_t time = Options::getShort( MQTT_MODULE, "Telemetry", Config::MQTT_TELEMETRY_TIME );
  if( phase == 0 && time > 0 ) {
    telemetry_period++;
    if( telemetry_period >= time ) {
      telemetry_period = 0;
      Modules.execute( MQTT_MODULE, [this](Module* module) {
        ((MqttClientModule*) module)->publish( MqttClientModule::TopicPrefix::TELE, "report", prepareTelemetry() );
      });
    }
  }
  // Check the invoked restart case.
  else if( phase == 6 && flags.restart_invoked ) {
    if( reconfig_delay_counter > 0 ) {
      reconfig_delay_counter -= 1;
    } else {
      flags.restart_invoked = false;
      ESP.restart();
    }
  }
}

/* Module Web interface */

const String CoreModule::getModuleWebpage() {
  return makeWebpage( "/module_core.html" );
}

/* A generic getData/setData interface */

const String CoreModule::getString( const String& key ) {
  SWITCH( key.c_str() ) {
    CASE( Config::KEY_EXPORT_CONFIGURATION ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["sleepmode"] = fromSleepMode( Options::getSleepMode() );
      json["sleeptime"] = Options::getSleepTime();
      json["loglevel"]  = fromLogLevel( getByteOption( "LogLevel", Config::LOG_LEVEL ));
      json["options"]   = getStringOption( MODULE_OPTIONS_KEY );
      return json.as<String>();
    }
    DEFAULT_CASE:
      return Module::getString( key );
  }
}

ResultData CoreModule::setString( const String& key, const String& value ) {
  SWITCH( key.c_str() ) {
    CASE( Config::KEY_IMPORT_CONFIGURATION ):
      return handleConfigImport( value );
    DEFAULT_CASE:
      return Module::setString( key, value );
  }
}

/* Data upload interface. Used by WebServerModule to upload firmware. */

ResultData CoreModule::onDataUploadBegin( const String& action, int data_size ) {
  // Pre-check: isn't firmware upgrade already started.
  if( uploadHandler ) {
    return {RC_ERROR, Messages::UPLOAD_IN_PROGRESS};
  }
  // Pre-check: data_size should be > 0.
  if( data_size <= 0 ) {
    return {RC_ERROR, Messages::UPLOAD_INVALID_SIZE};
  }
  // Start the requested action.
  SWITCH( action.c_str() ) {
    CASE( "firmware" ): {
      uploadHandler = new FirmwareUploader();
      return uploadHandler->begin( action, data_size );
    }
    CASE( "fs_image" ): {
      uploadHandler = new FirmwareUploader();
      return uploadHandler->begin( action, data_size );
    }
    CASE( "config" ): {
      uploadHandler = new ConfigImporter();
      return uploadHandler->begin( action, data_size );
    }
    DEFAULT_CASE:
      return {RC_ERROR, Messages::COMMAND_UNKNOWN};
  }
}

ResultData CoreModule::onDataUploadNextBlock( const char* data, const uint16_t size ) {
  if( !uploadHandler )
    return {RC_ERROR, Messages::COMMAND_UNKNOWN};

  ResultData result = uploadHandler->uploadDataBlock( data, size );
  // if( result.code == RC_OK ) {
  //   StatusLED::LEDMode new_mode = onboardLED.getMode() == StatusLED::OFF ? StatusLED::ON : StatusLED::OFF;
  //   onboardLED.continuous( new_mode );
  // }
  return result;
}

ResultData CoreModule::onDataUploadEnd( const bool hasSuccessful ) {
  if( !uploadHandler ) {
    return {RC_ERROR, Messages::COMMAND_UNKNOWN};
  }
  ResultData result = uploadHandler->end( hasSuccessful );
  finalizeDataUpload();
  performPendingRestart();
  return result;
}

void CoreModule::performPendingRestart() {
  flags.restart_invoked = true;
  reconfig_delay_counter = 2;
}

/* Protected */

bool CoreModule::handleCommand( const String& cmd, const String& args ) {
  SWITCH( cmd.c_str() ) {
    // ==========================================
    // Heap memory statistics
    CASE( "heapstat" ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["HeapSize"] = ESP.getHeapSize();
      json["Free"]     = ESP.getFreeHeap();
      json["Lowest"]   = ESP.getMinFreeHeap();
      json["MaxBlock"] = ESP.getMaxAllocHeap();
      handleCommandResults( cmd, args, json.as<String>() );
      return true;
    }
    // ==========================================
    // Heap memory statistics
    CASE( "nvsstat" ): {
      nvs_stats_t nvs_stats;
      nvs_get_stats( NULL, &nvs_stats );
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["Used"] = nvs_stats.used_entries;
      json["Free"] = nvs_stats.free_entries;
      handleCommandResults( cmd, args, json.as<String>() );
      return true;
    }
    // ==========================================
    // Return the list (JSON array) of active modules.
    CASE( "modules" ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      JsonArray array = json.createNestedArray( "modules" );
      Modules.iterator( [&array](Module* module) {
        array.add( module->getId() );
      });
      handleCommandResults( cmd, args, json.as<String>() );
      return true;
    }
    // ==========================================
    // Get network info
    CASE( "netinfo" ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["LocalIP"] = WiFi.localIP().toString();
      json["SubnetMask"] = WiFi.subnetMask().toString();
      json["GatewayIP"] = WiFi.gatewayIP().toString();
      json["DnsIP"] = WiFi.dnsIP().toString();
      json["Mac"] = WiFi.macAddress();
      handleCommandResults( cmd, args, json.as<String>() );
      return true;
    }
    // ==========================================
    // Commands to manage persistent options (NVS) stored in non-volatile RAM.
    // options erase - Used to erase ALL persistent options. You must reconfigure the device afterwards.
    //                 Warning!! There is no extra confirmations for console commands, so the options will be erased immediately.
    CASE( "nvs" ): {
      if( args == "clear" ) {
        Options::clear();
        performPendingRestart();
      }
      return true;
    }
    // ==========================================
    // Get sleep and cpu load info
    CASE( "powerinfo" ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["SleepMode"] = fromSleepMode( Options::getSleepMode() );
      json["SleepTime"] = Options::getSleepTime();
      json["LoadAvg"] = State.cpuLoadValue();
      json["WiFiSleep"] = WiFi.getSleep();
      handleCommandResults( cmd, args, json.as<String>() );
      return true;
    }
    // ==========================================
    // Get some debug/diagnostic info
    CASE( "resetinfo" ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["RestartReason"] = Utils::getResetReason();
      handleCommandResults( cmd, args, json.as<String>() );
      return true;
    }
    // ==========================================
    // Restart
    // value 0 - System restart
    CASE( "restart" ): {
      handleCommandResults( cmd, args, Messages::OK );
      performPendingRestart();
      return true;
    }
    // ==========================================
    // Get the telemetry info
    CASE( "telemetry" ): {
      handleCommandResults( cmd, args, prepareTelemetry() );
      return true;
    }
    // ==========================================
    // Get system time and uptime info
    CASE( "timeinfo" ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      Modules.execute( RTC_MODULE, [&json](Module* module) {
        json["Local"] = ((RtcTimeModule*) module)->getLocalTimeString();
        json["Uptime"] = ((RtcTimeModule*) module)->getUptimeString();
      });
      handleCommandResults( cmd, args, json.as<String>() );
      return true;
    }
    // ==========================================
    // Get firmware version
    CASE( "version" ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["Codename"] = Config::PROJECT_NAME;
      json["Version"] = Utils::getSystemVersionName();
      handleCommandResults( cmd, args, json.as<String>() );
      return true;
    }
    // ==========================================
    // Get wifi info
    CASE( "wifiinfo" ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["SSID"] = WiFi.SSID();
      json["BSSID"] = WiFi.BSSIDstr();
      json["Channel"] = WiFi.channel();
      json["RSSI"] = Utils::getWifiRssiAsQuality( WiFi.RSSI() );
      handleCommandResults( cmd, args, json.as<String>() );
      return true;
    }
    // ==========================================
    // Unknown command
    DEFAULT_CASE:
      return false;
  }
}

ResultData CoreModule::handleOption( const String& key, const String& value, Options::Action action ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    CASE( "loglevel" ):
      if( action != Options::READ ) {
        uint8_t v = toLogLevel( value );
        if( v == 255 ) return INVALID_VALUE;
        if( action == Options::SAVE ) {
          setByteOption( "LogLevel", v );
          Modules.execute( LOG_MODULE, [v](Module* module) {
            ((LogManagerModule*) module)->setLogLevel( v );
          });
        }
      }
      return {RC_OK, fromLogLevel( getByteOption( "LogLevel", Config::LOG_LEVEL ))};
    // ==========================================
    CASE( "options" ): {
      auto rc = applyExtraOptions( value );
      if( rc.code != RC_OK ) {
        return rc;
      }
      setStringOption( MODULE_OPTIONS_KEY, value );
      return {RC_OK, value};
    }
    // ==========================================
    CASE( "sleepmode" ):
      if( action != Options::READ ) {
        uint8_t v = toSleepMode( value );
        if( v == 255 ) return INVALID_VALUE;
        if( action == Options::SAVE ) {
          Options::setSleepMode( v );
        }
      }
      return {RC_OK, fromSleepMode( Options::getSleepMode() )};
    // ==========================================
    CASE( "sleeptime" ):
      if( action != Options::READ ) {
        if( !Utils::isNumber( value.c_str() ))  return INVALID_VALUE;
        uint8_t v = atoi( value.c_str() );
        if( v > 250 ) return INVALID_VALUE;
        if( action == Options::SAVE ) {
          Options::setSleepTime( v );
        }
      }
      return {RC_OK, String( Options::getSleepTime() )};
    // ==========================================
    // Options with a key equals to custom module ID are used to dynamically
    // instantiate or destroy that module.
    DEFAULT_CASE:
      if( Modules.isCustomModule( key ) && Utils::isBool( value )) {
        const bool oldValue = getByteOption( key, 1 );
        const bool newValue = Utils::toBool( value );
        if( action == Options::SAVE && oldValue != newValue ) {
          setByteOption( key, newValue );
          // Instantiate or remove the module (if needed)
          Module* module = Modules.get( key );
          if( newValue && module == nullptr ) {
            Modules.add( key );
            Log.verbose( "CORE The %s module is added" CR, key.c_str() );
          } else if( !newValue && module ) {
            Modules.remove( key );
            Log.verbose( "CORE The %s module is removed" CR, key.c_str() );
          }
        }
        return {RC_OK, String( newValue )};
      }
      return UNKNOWN_OPTION;
  }
}

void CoreModule::resolveTemplateKey( const String& key, String& out ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    // loglevel
    CASE( "LOG_0" ):
      out += getByteOption( "LogLevel", Config::LOG_LEVEL ) == LOG_LEVEL_SILENT ? "selected" : "";
      break;
    CASE( "LOG_1" ):
      out += getByteOption( "LogLevel", Config::LOG_LEVEL ) == LOG_LEVEL_FATAL ? "selected" : "";
      break;
    CASE( "LOG_2" ):
      out += getByteOption( "LogLevel", Config::LOG_LEVEL ) == LOG_LEVEL_ERROR ? "selected" : "";
      break;
    CASE( "LOG_3" ):
      out += getByteOption( "LogLevel", Config::LOG_LEVEL ) == LOG_LEVEL_WARNING ? "selected" : "";
      break;
    CASE( "LOG_4" ):
      out += getByteOption( "LogLevel", Config::LOG_LEVEL ) == LOG_LEVEL_NOTICE ? "selected" : "";
      break;
    CASE( "LOG_5" ):
      out += getByteOption( "LogLevel", Config::LOG_LEVEL ) == LOG_LEVEL_TRACE ? "selected" : "";
      break;
    CASE( "LOG_6" ):
      out += getByteOption( "LogLevel", Config::LOG_LEVEL ) == LOG_LEVEL_VERBOSE ? "selected" : "";
      break;
    // ==========================================
    // sleepmode
    CASE( "SM_DYN" ):
      out += Options::getSleepMode() == Config::SleepMode::DYNAMIC ? "selected" : "";
      break;
    CASE( "SM_STA" ):
      out += Options::getSleepMode() == Config::SleepMode::STATIC ? "selected" : "";
      break;
    // ==========================================
    // sleeptime
    CASE( "SM_TIME" ):
      out += Options::getSleepTime();
      break;
    CASE( "MANAGE_MD" ):
      out += prepareManageModulesHTML();
      break;
    CASE( "TITLE" ):
      out += Utils::formatModuleSettingsTitle( getId(), getName() );
      break;
    CASE( "OPTIONS" ):
      out += getStringOption( MODULE_OPTIONS_KEY );
      break;
  }
}

/* Private */

ResultData CoreModule::applyExtraOptions( const String& options ) {
  if( options.length() > 0 )  {
    DynamicJsonDocument doc( Config::JSON_CONFIG_SIZE );
    DeserializationError rc = deserializeJson( doc, options );
    if( rc != DeserializationError::Ok ) {
      return {RC_ERROR, Utils::format( Messages::JSON_DECODE_ERROR, rc.c_str() )};
    }

    JsonObject json = doc.as<JsonObject>();

    // 1st step: validation

    // Optional Wire (I2C) initialization parameters.
    if( json.containsKey( WIRE_OPTION_KEY )) {
      const JsonObject wire = json[WIRE_OPTION_KEY].as<JsonObject>();
      auto rc = Utils::validateJsonNumber( wire, SDA_OPTION_KEY, 1, 39 );
      if( rc.code != RC_OK ) {
        return rc;
      }
      rc = Utils::validateJsonNumber( wire, SCL_OPTION_KEY, 1, 39 );
      if( rc.code != RC_OK ) {
        return rc;
      }
    }

    // 2nd step: apply options

    // Optional Wire (I2C) initialization parameters.
    if( json.containsKey( WIRE_OPTION_KEY )) {
      const JsonObject wire = json[WIRE_OPTION_KEY].as<JsonObject>();
      const uint8_t sda = wire[SDA_OPTION_KEY].as<int>();
      const uint8_t scl = wire[SCL_OPTION_KEY].as<int>();
      if( Wire.begin( sda, scl )) {
        Log.verbose( "CORE i2c wire is configured (scl=%d, sda=%d)" CR, scl, sda );
      } else {
        return {RC_ERROR, Messages::I2C_INIT_ERROR};
      }
    }
  }
  return RESULT_OK;
}

void CoreModule::finalizeDataUpload() {
  if( uploadHandler ) {
    delete uploadHandler;
    uploadHandler = nullptr;
    //onboardLED.continuous( StatusLED::OFF );
  }
}

const char* MODULE_SWITCH_HTML = R"(<div class="custom-control custom-switch mb-1">)"
                                 R"(<input type="checkbox" class="custom-control-input" id="SW%s" name="%s" value="true" %s>)"
                                 R"(<label class="custom-control-label" for="SW%s">Enable the %s module</label>)"
                                 R"(</div>)";

String CoreModule::prepareManageModulesHTML() {
  String result;
  const uint8_t count = sizeof(CUSTOM_MODULE_IDS) / sizeof(CUSTOM_MODULE_IDS[0]);
  for( uint8_t i = 0; i < count; i++ ) {
    const char* id = CUSTOM_MODULE_IDS[i];
    const char* name = CUSTOM_MODULE_NAMES[i];
    char buffer[500];
    const char* checked = getByteOption( id ) ? "checked" : "";
    snprintf( buffer, sizeof(buffer), MODULE_SWITCH_HTML, id, id, checked, id, name );
    result += buffer;
  }
  return result;
}

/* Private static */

String CoreModule::prepareTelemetry() {
  StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
  Modules.execute( RTC_MODULE, [&json](Module* module) {
    json["Time"] = ((RtcTimeModule*) module)->getLocalTimeIsoString();
    json["Uptime"] = ((RtcTimeModule*) module)->getUptimeString();
  });
  json["LoadAvg"] = State.cpuLoadValue();
  return json.as<String>();
}

void CoreModule::publishMqttConnectionInfo1() {
  StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
  json["Codename"] = Config::PROJECT_NAME;
  json["Version"] = Utils::getSystemVersionName();
  json["RestartReason"] = Utils::getResetReason();
  Modules.execute( MQTT_MODULE, [&json](Module* module) {
    ((MqttClientModule*) module)->publish( MqttClientModule::TopicPrefix::TELE, "info1", json );
  });
}

void CoreModule::publishMqttConnectionInfo2() {
  StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
  json["Hostname"]      = getMacroOptionOf( WIFI_MODULE, "Hostname", Config::WIFI_HOSTNAME );
  json["LocalIP"]       = WiFi.localIP().toString();
  json["FallbackTopic"] = getMacroOptionOf( MQTT_MODULE, "ClientId", Config::MQTT_CLIENT_ID );
  json["GroupTopic"]    = getMacroOptionOf( MQTT_MODULE, "GrpTopic", Config::MQTT_GROUP_TOPIC );
  Modules.execute( MQTT_MODULE, [&json](Module* module) {
    ((MqttClientModule*) module)->publish( MqttClientModule::TopicPrefix::TELE, "info2", json );
  });
}

/* Private static */

const char* CoreModule::fromLogLevel( uint8_t level ) {
  switch( level ) {
    case 1:   return "fatal";
    case 2:   return "error";
    case 3:   return "warning";
    case 4:   return "notice";
    case 5:   return "trace";
    case 6:   return "verbose";
    default:  return "silent";
  }
}

const char* CoreModule::fromSleepMode( uint8_t mode ) {
  return mode == Config::SleepMode::STATIC ? "static" : "dynamic";
}

uint8_t CoreModule::toSleepMode( const String& value ) {
  if( value == "dynamic" ) {
    return Config::SleepMode::DYNAMIC;
  }
  else if( value == "static" ) {
    return Config::SleepMode::STATIC;
  }
  else  {
    return 255;   // Indicates an invalid value.
  }
}

uint8_t CoreModule::toLogLevel( const String& value ) {
  SWITCH( value.c_str() ) {
    CASE( "silent" ):   return 0;
    CASE( "fatal" ):    return 1;
    CASE( "error" ):    return 2;
    CASE( "warning" ):  return 3;
    CASE( "notice" ):   return 4;
    CASE( "trace" ):    return 5;
    CASE( "verbose" ):  return 6;
    DEFAULT_CASE:       return 255;
  }
}