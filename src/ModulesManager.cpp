#include <ArduinoLog.h>
#include "str_switch.h"
#include "CoreModule.h"
#include "CustomConfig.h"
#include "Events.h"
#include "LogManagerModule.h"
#include "ModulesManager.h"
#include "MqttClientModule.h"
#include "RtcTimeModule.h"
#include "StatusLedModule.h"
#include "Utils.h"
#include "WebServerModule.h"
#include "WifiModule.h"

#ifdef USE_AM312_MODULE
  #include "AM312Module.h"
#endif
#ifdef USE_BACKLIGHT_MODULE
  #include "backlight/BackLightModule.h"
#endif
#ifdef USE_BH1750_MODULE
  #include "BH1750Module.h"
#endif
#ifdef USE_BME280_MODULE
  #include "BME280Module.h"
#endif
#ifdef USE_CLOCK1_MODULE
  #include "LedClock1Module.h"
#endif
#ifdef USE_MINI_DISPLAY_MODULE
  #include "minidisplay/MiniDisplayModule.h"
#endif
#ifdef USE_OPEN_WEATHER_MAP_MODULE
  #include "OpenWeatherMapModule.h"
#endif
#ifdef USE_RELAYS_MODULE
  #include "RelaysModule.h"
#endif
#ifdef USE_INFRARED_TRANSMITTER_MODULE
  #include "InfraredTransmitter.h"
#endif
#ifdef USE_ST7796_MODULE
  #include "ST7796Module.h"
#endif


/* Extern */

ModulesManager Modules;

/* Public */

ModulesManager::ModulesManager() {
  modules = new SimpleMap<String, Module*>( [](String& a, String& b) -> int {
    return b.compareTo( a );
  });
}

void ModulesManager::add( const String& moduleId ) {
  if( !modules->has( moduleId )) {
    Module* m = create( moduleId );
    if( m != NULL ) {
      modules->put( moduleId, m );
    }
  }
}

void ModulesManager::add( std::initializer_list<String> moduleIds ) {
  for( auto module : moduleIds ) {
    add( module );
  }
}

void ModulesManager::dispatchCommand( const String& command ) {
  // Try to find the module which ID equals to key.
  auto pair = Utils::split( command );
  const int count = Modules.count();
  for( int i = 0; i < count; i++ ) {
    Module* module = Modules.get( i );
    if( strcmp( pair.first.c_str(), module->getId() ) == 0 ) {
      // Module found, forward command to the module.
      module->dispatchCommand( pair.second );
      return;
    }
  }
  // Module is not found. Maybe the command doesn't begins with a module ID.
  // In this case forward it to the core module.
  Module* module = get( CORE_MODULE );
  if( module ) {
    module->dispatchCommand( command );
  }
}

void ModulesManager::execute( const String& moduleId, ModuleCallback callback ) {
  if( modules->has( moduleId )) {
    Module* m = get( moduleId );
    callback( m );
  }
}

Module* ModulesManager::get( const String& moduleId ) {
  return modules->has( moduleId ) ? modules->get( moduleId ) : nullptr;
}

bool ModulesManager::isCustomModule( const String& moduleId ) {
  for( auto id : CUSTOM_MODULE_IDS ) {
    if( moduleId == id ) {
      return true;
    }
  }
  return false;
}

void ModulesManager::iterator( ModuleCallback callback ) {
  for( int i = modules->size() - 1; i >= 0; i-- ) {
    Module* module = get( i );
    callback( module );
  }
}

void ModulesManager::remove( const String& moduleId ) {
  Module* m = get( moduleId );
  if( m ) {
    modules->remove( moduleId );
    delete m;
  }
}

/* Private */

/**
 * A factory method to create an instance of specified module.
 */
Module* ModulesManager::create( const String& module ) {
  SWITCH( module.c_str() ) {
  // -- Core modules --------------------------------
    CASE( CORE_MODULE ):              return new CoreModule();
    CASE( LOG_MODULE ):               return new LogManagerModule();
    CASE( MQTT_MODULE ):              return new MqttClientModule();
    CASE( RTC_MODULE ):               return new RtcTimeModule();
    CASE( WEBSERVER_MODULE ):         return new WebServerModule();
    CASE( WIFI_MODULE ):              return new WifiModule();
  // -- Custom modules ------------------------------
  #ifdef USE_AM312_MODULE
    CASE( AM312_PIR_MODULE ):         return new AM312Module();
  #endif
  #ifdef USE_BACKLIGHT_MODULE
    CASE( BACKLIGHT_MODULE ):         return new BackLightModule();
  #endif
  #ifdef USE_BH1750_MODULE
    CASE( BH1750_MODULE ):            return new BH1750Module();
  #endif
  #ifdef USE_BME280_MODULE
    CASE( BME280_MODULE ):            return new BME280Module();
  #endif
  #ifdef USE_CLOCK1_MODULE
    CASE( CLOCK1_MODULE ):            return new LedClock1Module();
  #endif
  #ifdef USE_MINI_DISPLAY_MODULE
    CASE( MINI_DISPLAY_MODULE ):      return new MiniDisplayModule();
  #endif
  #ifdef USE_OPEN_WEATHER_MAP_MODULE
    CASE( OPEN_WEATHER_MAP_MODULE ):  return new OpenWeatherMapModule();
  #endif
  #ifdef USE_RELAYS_MODULE
    CASE( RELAYS_MODULE ):            return new RelaysModule();
  #endif
  #ifdef USE_ST7796_MODULE
    CASE( ST7796_MODULE ):            return new ST7796Module();
  #endif
  #ifdef USE_INFRARED_TRANSMITTER_MODULE
    CASE( INFRARED_MODULE ):          return new InfraredModule();
  #endif
  #ifdef USE_STATUS_LED_MODULE
    CASE( STATUS_LED_MODULE ):        return new StatusLedModule();
  #endif
    DEFAULT_CASE:                     return nullptr;
  }
}
