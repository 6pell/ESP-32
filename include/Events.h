#pragma once
#include <EventBus.h>
#include <WString.h>
#include "Module.h"

// https://github.com/gelldur/EventBus

/*******************************************************************************/
/* Event bus events */

/* CommandResponseEvent */

struct CommandResponseEvent {
  Module* const module;
  const String& topic;
  const String& results;
};

/* ConnectivityEvent */

struct ConnectivityEvent {
  enum Type{ TYPE_WIFI, TYPE_MQTT, TYPE_ACCESS_POINT };
  //
  const Type type;
  const bool connected;
};

/* LogUpdateEvent */

struct LogUpdateEvent {};

/* StatusChangedEvent */

struct StatusChangedEvent {
  Module* const module;       // The module ID.
  const String& payload;      // An optional payload. The data format is module-specific.
};

/* SystemEvent */

struct SystemEvent {
  enum Type{ TYPE_PENDING_RESTART };
  //
  const Type type;
};

/*******************************************************************************/
/* GlobalState */

class GlobalState {
private:

  typedef union {
    uint8_t data;
    struct {
      uint8_t wifi_connected   : 1;
      uint8_t mqtt_connected   : 1;
      uint8_t web_manager_mode : 1;
      uint8_t spare03          : 1;
      uint8_t spare04          : 1;
      uint8_t spare05          : 1;
      uint8_t spare06          : 1;
      uint8_t spare07          : 1;
    };
  } StateFlags;

  StateFlags flags;
  uint32_t cpuLoad = 0;

public:
  // Constructor
  GlobalState();
  // Getters
  const uint32_t cpuLoadValue()    { return cpuLoad; }
  const bool mqttConnected()       { return flags.mqtt_connected; }
  const bool wifiConnected()       { return flags.wifi_connected; }
  const bool webManagerMode()      { return flags.web_manager_mode; }
  // Setters
  void updateCpuLoad( uint32_t activity_time, uint32_t sleep_time );
};

extern Dexode::EventBus Bus;
extern GlobalState State;
