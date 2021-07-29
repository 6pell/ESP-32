#pragma once
#include "CustomConfig.h"
#include "Messages.h"

// Module identifiers, no more than 9 chars.
// Modules are addressed in a console by their IDs.
constexpr const char* CORE_MODULE             = "core";
constexpr const char* LOG_MODULE              = "log";
constexpr const char* WIFI_MODULE             = "wifi";
constexpr const char* MQTT_MODULE             = "mqtt";
constexpr const char* RTC_MODULE              = "rtc";
constexpr const char* WEBSERVER_MODULE        = "webserver";

constexpr const char* AM312_PIR_MODULE        = "am312";
constexpr const char* BACKLIGHT_MODULE        = "backlight";
constexpr const char* BH1750_MODULE           = "bh1750";
constexpr const char* BME280_MODULE           = "bme280";
constexpr const char* CLOCK1_MODULE           = "clock1";
constexpr const char* MINI_DISPLAY_MODULE     = "display";
constexpr const char* OPEN_WEATHER_MAP_MODULE = "weather";
constexpr const char* RELAYS_MODULE           = "relays";
constexpr const char* ST7796_MODULE           = "st7796";
constexpr const char* STATUS_LED_MODULE       = "led";

// Warning!
// MODULE_IDS and MODULE_NAMES arrays must be respectively related to each other.

static constexpr const char* const CORE_MODULE_IDS[] = {
  CORE_MODULE,
  LOG_MODULE,
  WIFI_MODULE,
  MQTT_MODULE,
  RTC_MODULE,
  WEBSERVER_MODULE
};

static constexpr const char* const CORE_MODULE_NAMES[] = {
  Messages::TITLE_CORE_MODULE,
  "",
  Messages::TITLE_WIFI_MODULE,
  Messages::TITLE_MQTT_MODULE,
  Messages::TITLE_RTC_MODULE,
  Messages::TITLE_WEBSERVER_MODULE
};

static constexpr const char* const CUSTOM_MODULE_IDS[] = {
  #ifdef USE_AM312_MODULE
    AM312_PIR_MODULE,
  #endif
  #ifdef USE_BACKLIGHT_MODULE
    BACKLIGHT_MODULE,
  #endif
  #ifdef USE_BH1750_MODULE
    BH1750_MODULE,
  #endif
  #ifdef USE_BME280_MODULE
    BME280_MODULE,
  #endif
  #ifdef USE_CLOCK1_MODULE
    CLOCK1_MODULE,
  #endif
  #ifdef USE_MINI_DISPLAY_MODULE
    MINI_DISPLAY_MODULE,
  #endif
  #ifdef USE_OPEN_WEATHER_MAP_MODULE
    OPEN_WEATHER_MAP_MODULE,
  #endif
  #ifdef USE_RELAYS_MODULE
    RELAYS_MODULE,
  #endif
  #ifdef USE_ST7796_MODULE
    ST7796_MODULE,
  #endif
  #ifdef USE_STATUS_LED_MODULE
    STATUS_LED_MODULE
  #endif
};

static constexpr const char* const CUSTOM_MODULE_NAMES[] = {
  #ifdef USE_AM312_MODULE
    Messages::TITLE_AM312_MODULE,
  #endif
  #ifdef USE_BACKLIGHT_MODULE
    Messages::TITLE_BACKLIGHT_MODULE,
  #endif
  #ifdef USE_BH1750_MODULE
    Messages::TITLE_BH1750_MODULE,
  #endif
  #ifdef USE_BME280_MODULE
    Messages::TITLE_BME280_MODULE,
  #endif
  #ifdef USE_CLOCK1_MODULE
    Messages::TITLE_CLOCK1_MODULE,
  #endif
  #ifdef USE_MINI_DISPLAY_MODULE
    Messages::TITLE_MINI_DISPLAY_MODULE,
  #endif
  #ifdef USE_OPEN_WEATHER_MAP_MODULE
    Messages::TITLE_OPEN_WEATHER_MAP_MODULE,
  #endif
  #ifdef USE_RELAYS_MODULE
    Messages::TITLE_RELAYS_MODULE,
  #endif
  #ifdef USE_ST7796_MODULE
    Messages::TITLE_ST7796_MODULE,
  #endif
  #ifdef USE_STATUS_LED_MODULE
    Messages::TITLE_STATUS_LED_MODULE
  #endif
};