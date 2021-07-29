#pragma once
#include <ArduinoLog.h>
#include <stdint.h>

// -- The project global configuration --------------

namespace Config {

  enum SleepMode         {STATIC, DYNAMIC};

  enum TemperatureScale  {UNKNOWN_SCALE, CELSIUS, FARENHEIT};

  enum WeatherUnits      {UNKNOWN_UNITS, METRIC, IMPERIAL};

  /**
   *  WifiConfigMethod
   *  0  Restart the device.
   *  1  Start wifi manager (web server at 192.168.4.1). After some inactivity interval return to client mode.
   *  2  Disable wifi config but retry other AP without restart.
   *  3  Disable wifi config but retry the same AP without restart.
   */
  enum WifiConfigMethod  { WIFI_RESTART, WIFI_MANAGER, WIFI_RETRY, WIFI_WAIT, WIFI_INVALID = 99 };


  // -- Application ---------------------------------
  const char* const      PROJECT_NAME               = "Project Shortsword";             //
  const uint32_t         VERSION_CODE               = 0x01001300;                       // 1.0.19

  const int              APP_SERIAL_BAUDRATE        = 115200;                           // Default serial port baudrate.

  // -- AM312 pyroelectic sensor --------------------
  const uint8_t          AM312_PIN                  = 35;                               // Pin number to which an AM312 sensor is connected.

  // -- Backlight RGB strip -------------------------
  const uint16_t         BACKLIGHT_MAX_POWER_BUDGET = 1000;                             // RGB strip max power budget, in milliampers. Zero means unlimited current.
  const uint16_t         BACKLIGHT_PIXELS_COUNT     = 45;                               // Number of LEDs in a strip.
  const uint8_t          BACKLIGHT_PIN              = 23;                               // A strip control pin number.
  const bool             BACKLIGHT_WS8212B_ECO      = true;                             // True if WS8212B-ECO LED strip is used. Affects to the power consumption calculation.

  // -- BH1750 lux sensor ---------------------------
  const float            BH1750_LUX_DELTA           = 10.0;                             // Send an event if the lux changed more than delta.
  const uint16_t         BH1750_POLL_INTERVAL       = 10;                               // Measure values every xx sec.

  // -- BME280 temperature, etc sensor --------------
  const uint8_t          BME280_SENSOR_ADDR         = 0x76;                             // BME280 sensor i2c address.
  const uint16_t         BME280_POLL_INTERVAL       = 10;                               // Measure values every xx sec.
  const uint8_t          BME280_TEMPERATURE_SCALE   = TemperatureScale::CELSIUS;        // Temperature scale, CELSIUS or FARENHEIT.
  const float            BME280_TEMPERATURE_DELTA   = 0.1;                              // Send an event if the temperature changed more than delta.
  const float            BME280_HUMIDITY_DELTA      = 1.0;                              // Send an event if the humidity changed more than delta.

  // -- Log -----------------------------------------
  const uint8_t          LOG_LEVEL                  = LOG_LEVEL_VERBOSE;                // [loglevel] The log level
                                                                                        // LOG_LEVEL_SILENT     no output
                                                                                        // LOG_LEVEL_FATAL      fatal errors
                                                                                        // LOG_LEVEL_ERROR      all errors
                                                                                        // LOG_LEVEL_WARNING    errors, and warnings
                                                                                        // LOG_LEVEL_NOTICE     errors, warnings and notices
                                                                                        // LOG_LEVEL_TRACE      errors, warnings, notices & traces
                                                                                        // LOG_LEVEL_VERBOSE    all
  const int              LOG_MAX_LINES              = 14;

  // -- HTTP ----------------------------------------
  const char* const      WEB_SERVER_NAME            = PROJECT_NAME;                     // [name] Device name in web interface
  const uint16_t         WEB_SERVER_PORT            = 80;                               // [port] Web server port
  const char* const      WEB_SERVER_AP_SSID         = "ESP32-AP";                       // Wifi manager, access point mode SSID
  const char* const      WEB_SERVER_AP_PASSWORD     = "12345678";                       // Wifi manager, access point mode password

  const bool             WEB_AUTH_ENABLED           = false;                            // [auth] Web server authentication, enable/disable.
  const char* const      WEB_AUTH_USERNAME          = "admin";                          // [username] Web server authentication, user name
  const char* const      WEB_AUTH_PASSWORD          = "admin";                          // [password] Web server authentication, password

  const char* const      WEB_SESSION_COOKIE_NAME    = "ssid";                           // This is the name of the cookie carrying the session ID.
  const double           WEB_SESSION_TTL            = 10.0 * 60.0;                      // Sessions are destroyed after 10 minutes of inactivity.
  const double           WEB_SESSION_CHECK_INTERVAL = 1.0 * 60.0;                       // Sessions expiration is checked every minute.
  const uint8_t          WEB_MAX_SESSIONS           = 2;                                // A simple in-memory storage for just 2 sessions.

  // -- Mini display with 3 buttons keyboard --------
  const uint8_t          MINI_DISPLAY_SELECT_PIN    = 34;                               // Keyboard "select" pin
  const uint8_t          MINI_DISPLAY_DOWN_PIN      = 35;                               // Keyboard "down" pin
  const uint8_t          MINI_DISPLAY_UP_PIN        = 36;                               // Keyboard "up" pin
  const uint16_t         MINI_DISPLAY_TIMEOUT       = 120;                              // Timeout to turn off display, seconds

  // -- MQTT module -------------------------------
  const char* const      MQTT_CLIENT_ID             = "ESP32_#MAC4";                    // [clientid] MQTT client ID. Also a MQTT fallback topic.
  const char* const      MQTT_HOST                  = "192.168.0.4";                    // [host]
  const uint16_t         MQTT_PORT                  = 1883;                             // [port] MQTT port (10123 on CloudMQTT)
  const char* const      MQTT_USERNAME              = "";                               // [username] Optional user
  const char* const      MQTT_PASSWORD              = "";                               // [password] Optional password

  const uint16_t         MQTT_RECONNECT_TIME        = 10;                               // [reconnect] Minimum seconds to retry MQTT connection
  const uint16_t         MQTT_TELEMETRY_TIME        = 300;                              // [telemetry] Telemetry (0 = disable, 10 - 3600 seconds)

  const char* const      MQTT_DEVICE_TOPIC          = "device";                         // [topic] MQTT device topic, must be unique in a network
  const char* const      MQTT_GROUP_TOPIC           = "esp32";                          // [grptopic] MQTT Group topic
  const char* const      MQTT_SUB_PREFIX            = "cmnd";                           // [сmnd] Device subscribes to %prefix%/%topic%
  const char* const      MQTT_PUB_PREFIX            = "stat";                           // [stat] Device publishes to %prefix%/%topic%
  const char* const      MQTT_PUB_PREFIX2           = "tele";                           // [tele] Device publishes telemetry data to %prefix%/%topic%
  const char* const      MQTT_FULL_TOPIC            = "#PREFIX/#TOPIC";                 // [fulltopic] Subscribe and Publish full topic name

  // -- Nextion display module ----------------------
  #define                NEXTION_SERIAL               Serial1                           // Serial connection to Nextion display
  const int8_t           NEXTION_RX_PIN             = 26;                               // Serial port RX pin
  const int8_t           NEXTION_TX_PIN             = 27;                               // Serial port TX pin
  const uint32_t         NEXTION_BAUDRATE           = 115200;                           // Display baudrate (2400..115200)
  const uint8_t          NEXTION_BRIGHTNESS         = 99;                               // Display brightness (0..99)
  const uint8_t          NEXTION_PAGE               = 0;                                // Display page number used as defult during initialization.

  // -- OpenWeatherMap module -----------------------
  const char* const      WEATHER_EXCLUDE            = "minutely,hourly";
  const uint8_t          WEATHER_UNITS              = WeatherUnits::METRIC;

  // -- Relays module -------------------------------
  const uint8_t          RELAY_MAX_RELAYS           = 8;
  const uint8_t          RELAY1_PIN                 = 32;                               // Relay control pins
  const uint8_t          RELAY2_PIN                 = 33;
  const uint8_t          RELAY3_PIN                 = 25;
  const uint8_t          RELAY4_PIN                 = 14;
  const uint8_t          RELAY5_PIN                 = 26;
  const uint8_t          RELAY6_PIN                 = 27;
  const uint8_t          RELAY7_PIN                 = 0;
  const uint8_t          RELAY8_PIN                 = 0;
  const bool             RELAY1_INVERSE_PIN         = true;                             // Relay control pin inversion, useful if relay is switched by low logical level.
  const bool             RELAY2_INVERSE_PIN         = true;
  const bool             RELAY3_INVERSE_PIN         = true;
  const bool             RELAY4_INVERSE_PIN         = true;
  const bool             RELAY5_INVERSE_PIN         = true;
  const bool             RELAY6_INVERSE_PIN         = true;
  const bool             RELAY7_INVERSE_PIN         = true;
  const bool             RELAY8_INVERSE_PIN         = true;

  // -- Status LED module ---------------------------
  const int8_t           STATUS_LED_PIN             = LED_BUILTIN;                      // Pin to which the status indication LED is connected.
  const bool             STATUS_LED_INVERSE_PIN     = false;

  // -- ST7796 TFT display module -------------------
  const int8_t           ST7796_CS_PIN              = -1;                               // Chip select control pin (library pulls permanently low
  const uint8_t          ST7796_DC_PIN              = 15;                               // Data Command control pin - must use a pin in the range 0-31
  const int8_t           ST7796_RST_PIN             = -1;                               // Reset pin, toggles on startup
  const uint8_t          ST7796_WR_PIN              =  4;                               // Write strobe control pin - must use a pin in the range 0-31
  const uint8_t          ST7796_RD_PIN              = 22;                               // Read strobe control pin

  const uint8_t          ST7796_D0_PIN              = 12;                               // Must use pins in the range 0-31 for the data bus
  const uint8_t          ST7796_D1_PIN              = 13;                               // so a single register write sets/clears all bits.
  const uint8_t          ST7796_D2_PIN              = 26;                               // Pins can be randomly assigned, this does not affect
  const uint8_t          ST7796_D3_PIN              = 25;                               // TFT screen update performance.
  const uint8_t          ST7796_D4_PIN              = 17;
  const uint8_t          ST7796_D5_PIN              = 16;
  const uint8_t          ST7796_D6_PIN              = 27;
  const uint8_t          ST7796_D7_PIN              = 14;

  const int8_t           ST7796_BACKLIGHT_PIN       = 2;
  const uint8_t          ST7796_TOUCH_CS_PIN        = 5;
  const uint8_t          ST7796_TOUCH_IRQ_PIN       = 34;
  const uint8_t          ST7796_CLK_PIN             = 18;
  const uint8_t          ST7796_MOSI_PIN            = 23;
  const uint8_t          ST7796_MISO_PIN            = 19;

  const bool             ST7796_TOUCH_XY_SWAP       = false;
  const bool             ST7796_TOUCH_INVERT_X      = false;
  const bool             ST7796_TOUCH_INVERT_Y      = false;
  const uint16_t         ST7796_TOUCH_X_MIN         = 110;
  const uint16_t         ST7796_TOUCH_Y_MIN         = 110;
  const uint16_t         ST7796_TOUCH_X_MAX         = 1980;
  const uint16_t         ST7796_TOUCH_Y_MAX         = 1980;
  const uint8_t          ST7796_TOUCH_AVG           = 4;

  const uint8_t          ST7796_SDCARD_CS_PIN       = 21;

  // -- System --------------------------------------
  const uint8_t          SYSTEM_SLEEP_TIME          = 15;                               // [sleeptime] Sleep time to lower energy consumption (0 = Off .. 1-250 mSec)
  const uint8_t          SYSTEM_SLEEP_MODE          = DYNAMIC;                          // [sleepmode] Sleep mode (0 = Static, 1 = Dynamic)

  // -- Time - Up to three NTP servers in your region
  const char* const      RTC_NTP_SERVER1            = "0.ua.pool.ntp.org";              // [ntp1] Select first NTP server by name or IP address
  const char* const      RTC_NTP_SERVER2            = "0.pool.ntp.org";                 // [ntp2] Select second NTP server by name or IP address
  const char* const      RTC_NTP_SERVER3            = "1.pool.ntp.org";                 // [ntp3] Select third NTP server by name or IP address
  // -- Time - Timezone
  const char* const      RTC_TIMEZONE               = "EET-2EEST,M3.5.0/3,M10.5.0/4";   // [timezone]
  const uint16_t         RTC_GMT_OFFSET             = 3600 * 2;                         // [gmtoffset] GMT time offset, seconds
  const uint16_t         RTC_DST_OFFSET             = 3600;                             // [dstoffset] Daylight saving time offset, seconds

  // -- i2c wire interface --------------------------
  const uint8_t          WIRE_SCL                   = 32;                               //
  const uint8_t          WIRE_SDA                   = 33;                               //

  // -- Wifi ----------------------------------------
  // Update these with values suitable for your network.
  const char* const      WIFI_IP_ADDRESS            = "0.0.0.0";                        // [ipaddr] Set to 0.0.0.0 to use DHCP or specify an IP address
  const char* const      WIFI_GATEWAY               = "192.168.0.1";                    // [ipgate] If not using DHCP, set Gateway IP address
  const char* const      WIFI_SUBNET_MASK           = "255.255.255.0";                  // [ipmask] If not using DHCP, set Network mask
  const char* const      WIFI_DNS                   = "192.168.0.1";                    // [ipdns] If not using DHCP, set DNS IP address (might be equal to WIFI_GATEWAY)

  const char* const      WIFI_SSID1                 = "";                               // [ssid1] Wifi SSID
  const char* const      WIFI_PASSWORD1             = "";                               // [pwd1] Wifi password
  const char* const      WIFI_SSID2                 = "";                               // [ssid2] Optional alternate AP Wifi SSID
  const char* const      WIFI_PASSWORD2             = "";                               // [pwd2] Optional alternate AP Wifi password
  const char* const      WIFI_HOSTNAME              = "#TOPIC-#MAC4";                   // [hostname] Expands to <MQTT_TOPIC>-<last 4 decimal chars of MAC address>

  const WifiConfigMethod WIFI_CONFIG_METHOD         = WIFI_MANAGER;                     // [cfgmethod] Default configuration method when wifi fails to connect

  const uint16_t         WIFI_RECONFIG_MAX_TIME     = 60 * 3;                           // Seconds before restart
  const uint16_t         WIFI_CHECK_TIME            = 20;                               // Check connection every xx seconds
  const uint16_t         WIFI_INACTIVITY_CHECK_TIME = 60 * 5;                           // WiFi manager inactivity time in seconds. If no one is connected, WiFi will be reconfigured.
  const uint8_t          WIFI_CONNECT_RETRIES       = 20;                               // Number of retries to reconnect
  const uint8_t          WIFI_CONNECT_DELAY         = 4;                                // Seconds, reconnect time (one attempt)


  // -- Definitions ---------------------------------
  const unsigned int     JSON_CONFIG_SIZE           = 1536;
  const unsigned int     JSON_EXPORT_CONFIG_SIZE    = JSON_CONFIG_SIZE * 4;
  const unsigned int     JSON_MESSAGE_SIZE          = 256;
  const unsigned int     MAX_PREFERENCES_KEY_LENGTH = 15;
  const unsigned int     MAX_STRING_KEY_SIZE        = 9;                                // Max length of string key (settings, etc)
  const unsigned int     MAX_STRING_LINE_SIZE       = 100;                              // Max number of chars in a settings line

  // -- !! Don't modify any identifier below !! ------
  // -- getString / setString Identifiers -----------
  constexpr const char* const KEY_EXPORT_CONFIGURATION  = "Export";                     // Export the module config to JSON.
  constexpr const char* const KEY_IMPORT_CONFIGURATION  = "Import";                     // Import the module config from JSON.
  constexpr const char* const KEY_MENU_DATA             = "MenuData";                   // Load and save the mini display menu config.
}

// -- mDNS ----------------------------------------
//#define USE_DISCOVERY                               // Enable mDNS for the following services. Disable by //
//#define WEBSERVER_ADVERTISE                         // Provide access to webserver by name <Hostname>.local/
//#define MQTT_HOST_DISCOVERY                         // Find MQTT host server (overrides MQTT_HOST if found)
