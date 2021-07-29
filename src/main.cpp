#include <Arduino.h>
#include <ArduinoLog.h>
#include "CustomConfig.h"
#include "Events.h"
#include "ModulesManager.h"
#include "Options.h"
#include "Utils.h"

#include "WiFiModule.h"

unsigned long  state_100mS_timer = 0;
uint8_t        state_100mS_phase = 0;

void setup() {
  Serial.begin( Config::APP_SERIAL_BAUDRATE );
  Options::setupPreferences();

  // Configure modules.
  Modules.add( {LOG_MODULE, CORE_MODULE, WIFI_MODULE, MQTT_MODULE, RTC_MODULE, WEBSERVER_MODULE} );

  // Call the custom setup to configure optional modules.
  customSetup();

  // All modules are initialized and configured. let's connect to the network.
  Modules.execute( WIFI_MODULE, [](Module* module) {
    ((WifiModule*) module)->begin();
  });
}

void loop() {
  const uint32_t timestamp = micros();

  Modules.iterator( [](Module* module) {
    Module::Properties properties = module->getProperties();
    if( properties.loop_required ) {
      module->loop();
    }
  });

  if( Utils::isTimeReached( state_100mS_timer )) {
    Utils::setNextTimeInterval( state_100mS_timer, 100 );
    // Perform every 100 milliseconds...
    if( ++state_100mS_phase > 9 ) {
      state_100mS_phase = 0;
    }
    Modules.iterator( [](Module* module) {
      Module::Properties properties = module->getProperties();
      if( properties.tick_100mS_required ) {
        module->tick_100mS( state_100mS_phase );
      }
    });
  }

  uint32_t activity = micros() - timestamp;
  uint32_t sleepTime = Options::getSleepTime();
  if( sleepTime > 0 ) {
    if( Options::getSleepMode() == Config::SleepMode::STATIC ) {
      // https://github.com/esp8266/Arduino/issues/2021
      delay( sleepTime );
    } else {
      uint32_t act = activity / 1000;
      if( act < sleepTime ) {
        delay( sleepTime - act );
      }
    }
  }
  State.updateCpuLoad( activity, sleepTime * 1000 );
}