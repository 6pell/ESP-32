#include "CustomConfig.h"
#include "ModulesManager.h"
#include "Options.h"
#include "custom/MiniDisplayBehavior.h"
#include "custom/StatusLedBehavior.h"

void customSetup() {
  // ==========================================================================
  // Instantiate optional modules.
  #ifdef USE_AM312_MODULE
    if( Options::getByte( CORE_MODULE, AM312_PIR_MODULE )) {
      Modules.add( AM312_PIR_MODULE );
    }
  #endif
  #ifdef USE_BACKLIGHT_MODULE
    if( Options::getByte( CORE_MODULE, BACKLIGHT_MODULE )) {
      Modules.add( BACKLIGHT_MODULE );
    }
  #endif
  #ifdef USE_BH1750_MODULE
    if( Options::getByte( CORE_MODULE, BH1750_MODULE )) {
      Modules.add( BH1750_MODULE );
    }
  #endif
  #ifdef USE_BME280_MODULE
    if( Options::getByte( CORE_MODULE, BME280_MODULE )) {
      Modules.add( BME280_MODULE );
    }
  #endif
  #ifdef USE_CLOCK1_MODULE
    if( Options::getByte( CORE_MODULE, CLOCK1_MODULE )) {
      Modules.add( CLOCK1_MODULE );
    }
  #endif
  #ifdef USE_MINI_DISPLAY_MODULE
    if( Options::getByte( CORE_MODULE, MINI_DISPLAY_MODULE )) {
      Modules.add( MINI_DISPLAY_MODULE );
    }
  #endif
  #ifdef USE_OPEN_WEATHER_MAP_MODULE
    if( Options::getByte( CORE_MODULE, OPEN_WEATHER_MAP_MODULE )) {
      Modules.add( OPEN_WEATHER_MAP_MODULE );
    }
  #endif
  #ifdef USE_RELAYS_MODULE
    if( Options::getByte( CORE_MODULE, RELAYS_MODULE )) {
      Modules.add( RELAYS_MODULE );
    }
  #endif
  #ifdef USE_STATUS_LED_MODULE
    if( Options::getByte( CORE_MODULE, STATUS_LED_MODULE )) {
      Modules.add( STATUS_LED_MODULE );
    }
  #endif
  #ifdef USE_ST7796_MODULE
    if( Options::getByte( CORE_MODULE, ST7796_MODULE )) {
      Modules.add( ST7796_MODULE );
    }
  #endif

  // ==========================================================================
  // Behavioral code

  #ifdef USE_MINI_DISPLAY_MODULE
    MiniDisplayBehavior::useConnectionStatus();
    MiniDisplayBehavior::useTimeStatus();
    #ifdef USE_RELAYS_MODULE
      MiniDisplayBehavior::useRelaysModuleStatus();
    #endif
  #endif

  #ifdef USE_STATUS_LED_MODULE
    StatusLedBehavior::useConnectionStatus();
  #endif
}