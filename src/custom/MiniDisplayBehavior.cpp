#include "Events.h"
#include "ModulesManager.h"
#include "Utils.h"
#include "RtcTimeModule.h"
#include "minidisplay/MiniDisplayModule.h"

namespace MiniDisplayBehavior {

  // ==========================================================================
  //

  void useConnectionStatus() {
    // Initialize the mini display module.
    Modules.execute( MINI_DISPLAY_MODULE, [](Module* module) {
      MiniDisplayModule* const display = (MiniDisplayModule*) module;
      //MqttClientModule* const mqtt = (MqttClientModule*) Modules.get( MQTT_MODULE );

      // Resolve "get value" requests of editable menu entries.
      // display->setOnRequestValueListener(
      //   [mqtt](const String& menuId, const String& entryId) -> String {
      //     // TODO request the value from the head unit via MQTT
      //     mqtt->publish( Options::TopicPrefix::CMND, "getvalue", "{request json}" );
      //     return R"({"menuId":"root","entryId":"num","type":"number","value":60,"min":30,"max":240,"step":30})";
      //   }
      // );
      // display->setOnChangeValueListener(
      //   [](const String& menuId, const String& entryId, const String& value) {
      //     // TODO send the modified value to the head unit via MQTT
      //   }
      // );
      // Which menu and entry should be displayed at the beginning.
      display->showDefaultMenuEntry();
    });

    // Subscribe to connectivity change events. Update the mini display "%STATUS%"
    // menu entry with actual connection status.
    Bus.listen<ConnectivityEvent>( [](const ConnectivityEvent& event ) {
      Modules.execute( MINI_DISPLAY_MODULE, [&event](Module* module) {
        MiniDisplayModule* const display = (MiniDisplayModule*) module;
        switch( event.type ) {
          case ConnectivityEvent::TYPE_WIFI:
            if( event.connected && Modules.get( MQTT_MODULE ) == NULL ) {
              display->setTemplateParameter( "STATUS", "Connected\n" + Utils::getLocalIP() );
            } else if( !State.webManagerMode() ) {
              display->setTemplateParameter( "STATUS", "Disconnected" );
            }
            break;
          case ConnectivityEvent::TYPE_MQTT:
            if( event.connected ) {
              display->setTemplateParameter( "STATUS", "Connected\n" + Utils::getLocalIP() );
            } else if( !State.webManagerMode() ) {
              display->setTemplateParameter( "STATUS", "WiFi " + Utils::getLocalIP() + "\nMQTT disconnected" );
            }
            break;
          case ConnectivityEvent::TYPE_ACCESS_POINT:
            display->setTemplateParameter( "STATUS", "Access point mode\n192.168.4.1" );
            break;
        }
      });
    });
  }

  // ==========================================================================
  // Subscribes to relays module status change events. Updates the related mini display menu entries.

  void useRelaysModuleStatus() {
    Bus.listen<StatusChangedEvent>( [](const StatusChangedEvent& event) {
      // ================================
      // Handling status change events of the relays module.
      // This code updates the mini display template parameter, where:
      // - the key is a relay alias or relay ID (if the relay alias is empty);
      // - the value is a relay state.
      if( strcmp( event.module->getId(), RELAYS_MODULE ) == 0 ) {
        // Avoid the display flickering when relay switches a reactive payload.
        Wire.begin();
        //delay( 250 );
        //Wire.begin();

        Modules.execute( MINI_DISPLAY_MODULE, [&event](Module* module) {
          StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
          DeserializationError rc = deserializeJson( json, event.payload );
          if( rc == DeserializationError::Ok ) {
            const String id = json["id"];
            const String alias = json["alias"];
            const String state = json["state"];
            // Use the relay alias as a template parameter when it's possible.
            // To select the mini display menu entry, it's ID must be the same as relay alias or id.
            MiniDisplayModule* const display = (MiniDisplayModule*) module;
            if( alias.length() > 0 && alias != "null" ) {
              display->setTemplateParameter( alias, state );
              display->selectMenu( alias );
            } else {
              display->setTemplateParameter( id, state );
              display->selectMenu( id );
            }
          } else {
             Log.error( "BHVR Relays status event decode error: %s" CR, rc.c_str() );
          }
        });
      }
    });
  }

  // ==========================================================================
  // Subscribes to RTC module status change events. Updates the mini display time.

  void useTimeStatus() {
    Bus.listen<StatusChangedEvent>( [](const StatusChangedEvent& event) {
      if( strcmp( event.module->getId(), RTC_MODULE ) == 0 ) {
        Modules.execute( MINI_DISPLAY_MODULE, [&event](Module* module) {
          String datetime = event.payload;
          int i = datetime.indexOf( 'T' );
          String date = datetime.substring( 0, i );
          String time = datetime.substring( i+1, datetime.lastIndexOf( ':' ));
          (static_cast<MiniDisplayModule*>(module))->setTemplateParameter( "DATE", date );
          (static_cast<MiniDisplayModule*>(module))->setTemplateParameter( "TIME", time );
        });
      }
    });
  }
}