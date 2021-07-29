#pragma once

namespace Messages {

  constexpr const char* TITLE_MODULE_SETTINGS         = "%s settings";
  constexpr const char* TITLE_CORE_MODULE             = "Core / system";
  constexpr const char* TITLE_MQTT_MODULE             = "MQTT client";
  constexpr const char* TITLE_RTC_MODULE              = "Real-time clock";
  constexpr const char* TITLE_WEBSERVER_MODULE        = "Web server";
  constexpr const char* TITLE_WIFI_MODULE             = "WiFi client";

  constexpr const char* TITLE_AM312_MODULE            = "AM312 pyroelectic sensor";
  constexpr const char* TITLE_BACKLIGHT_MODULE        = "WS8212B LED strip";
  constexpr const char* TITLE_BH1750_MODULE           = "BH1750 light sensor";
  constexpr const char* TITLE_BME280_MODULE           = "BME280 temperature sensor";
  constexpr const char* TITLE_CLOCK1_MODULE           = "LED alarm clock";
  constexpr const char* TITLE_MINI_DISPLAY_MODULE     = "SSD1306 LCD display";
  constexpr const char* TITLE_NEXTION_DISPLAY_MODULE  = "Nextion display";
  constexpr const char* TITLE_OPEN_WEATHER_MAP_MODULE = "OpenWeatherMap";
  constexpr const char* TITLE_RELAYS_MODULE           = "Relays (x8) shield";
  constexpr const char* TITLE_ST7796_MODULE           = "ST7796 display";
  constexpr const char* TITLE_STATUS_LED_MODULE       = "Status LED";

  constexpr const char* AM_312_MOTION                 = "Motion: %s";
  constexpr const char* BME1750_LUX                   = "Lux: %s";
  constexpr const char* BME280_TEMPERATURE            = "Temperature: %s%s";
  constexpr const char* BME280_HUMIDITY               = "Humidity: %s%%";
  constexpr const char* COMMAND_INVALID_VALUE         = "Invalid value";
  constexpr const char* COMMAND_UNKNOWN               = "Unknown command";
  constexpr const char* EFFECT_ID_MISSED              = "Effect ID is missed";
  constexpr const char* EFFECT_CREATE_FAILED          = "Failed to create an effect";
  constexpr const char* EFFECT_SELECT                 = "Select an effect";
  constexpr const char* FAILED                        = "Failed";
  constexpr const char* JSON_DECODE_ERROR             = "JSON decode error: %s";
  constexpr const char* I2C_INIT_ERROR                = "I2C initialization error";
  constexpr const char* MODULE_ID_MISSED              = "The module_id is missed";
  constexpr const char* MODULE_NOT_FOUND              = "Module %s is not found";
  constexpr const char* NEXTION_BAUDRATE_FAILED       = "Failed to set baudrate";
  constexpr const char* OK                            = "Ok";
  constexpr const char* OTA_DONE                      = "Firmware upload done!";
  constexpr const char* PALETTE_SELECT                = "Select a palette";
  constexpr const char* REQUEST_PARAMETER_MISSED      = "Request parameter is missed: ";
  constexpr const char* SETTINGS_MISSED_VALUE         = "Missed value: ";
  constexpr const char* SETTINGS_INVALID_VALUE        = ": invalid value";
  constexpr const char* SETTINGS_SAVED_OK             = "Settings are applied";
  constexpr const char* SETTINGS_UNKNOWN_OPTION       = "Unknown option";
  constexpr const char* UNKNOWN_MENU_ENTRY_ID         = "Unknown menu or entry ID";
  constexpr const char* UPLOAD_BLOCK_FAILED           = "Data block upload failed";
  constexpr const char* UPLOAD_COMPLETE               = "Upload complete";
  constexpr const char* UPLOAD_INVALID_SIZE           = "Invalid upload size";
  constexpr const char* UPLOAD_INVALID_URL            = "Invalid URL";
  constexpr const char* UPLOAD_IN_PROGRESS            = "Upload in progress";
  constexpr const char* UPLOAD_STARTED                = "Upload started";

}