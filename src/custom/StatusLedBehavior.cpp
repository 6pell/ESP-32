#include "Events.h"
#include "ModulesManager.h"
#include "StatusLedModule.h"

namespace StatusLedBehavior {

  void useConnectionStatus(){
    Bus.listen<ConnectivityEvent>( [](const ConnectivityEvent& event ) {
      Modules.execute( STATUS_LED_MODULE, [&event](Module* module) {
        StatusLedModule* const led = (StatusLedModule*) module;
        switch( event.type ) {
          case ConnectivityEvent::TYPE_WIFI:
            if( event.connected ) {
              // Turn off state LED only when derived modules like MQTT are disabled.
              if( Modules.get( MQTT_MODULE ) == nullptr ) {
                led->off();
              }
            } else if( !State.webManagerMode() ) {
              // Wifi problem so blink every second
              led->continuous( StatusLedModule::LEDMode::SLOW_BLINK );
            }
            break;

          case ConnectivityEvent::TYPE_MQTT:
            if( event.connected ) {
              led->off();
            } else if( !State.webManagerMode() ) {
              // MQTT problem so blink every 0.5 seconds
              led->continuous( StatusLedModule::LEDMode::FAST_BLINK );
            }
            break;

          case ConnectivityEvent::TYPE_ACCESS_POINT:
            led->continuous( StatusLedModule::LEDMode::FAST_FLASH );
            break;
        }
      });
    });
  }
}