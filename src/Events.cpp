#include <ArduinoLog.h>
#include "Events.h"
#include "Options.h"

Dexode::EventBus Bus;
GlobalState State;

/*******************************************************************************/
/* GlobalState */

GlobalState::GlobalState() {
  flags.data = 0;

  Bus.listen<ConnectivityEvent>( [this](const ConnectivityEvent& event) {
    switch( event.type ) {
      case ConnectivityEvent::TYPE_WIFI:
        flags.wifi_connected = event.connected;
        break;

      case ConnectivityEvent::TYPE_MQTT:
        flags.mqtt_connected = event.connected;
        break;

      case ConnectivityEvent::TYPE_ACCESS_POINT:
        flags.web_manager_mode = event.connected;
        break;
    }
  });
}

void GlobalState::updateCpuLoad( uint32_t activity_time, uint32_t sleep_time ) {
  if( sleep_time == 0 ) sleep_time = 1;
  cpuLoad = (100 * activity_time) / sleep_time;
}
