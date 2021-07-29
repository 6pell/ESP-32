#pragma once

#include <stdint.h>
#include "Module.h"

class WifiModule : public Module {
public:

/* ReconnectMode */
enum ReconnectMode { AP1, AP2, TOGGLE_AP, CURRENT_AP };

private:

/* StateFlags */
typedef union {
  uint8_t data;
  struct {
    uint8_t debug_log_enabled   : 1;
    uint8_t web_manager_started : 1;
    uint8_t invoke_reconfig     : 1;
    uint8_t invoke_restart      : 1;
    uint8_t spare04             : 1;
    uint8_t spare05             : 1;
    uint8_t spare06             : 1;
    uint8_t spare07             : 1;
  };
} StateFlags;

/* WifiModule */
private:
  StateFlags  flags;
  uint8_t     reconfigDelayCounter = 0;          // Seconds, delay before reconfigure issued by config method.
  uint16_t    checkConnectionDelayCounter;       // Seconds, delay between connection checks.
  uint8_t     active_ap = 0;                     // Active AP profile (i.e. active SSID and Pwd)
  uint8_t     connectionStatus;                  // WiFi connection status: 0=initial status; WL_CONNECTED
  uint8_t     connectionRetries;                 // Number of reconnection retries

public:
  WifiModule();
  virtual ~WifiModule();
  virtual void             tick_100mS( uint8_t phase );
  // Module identification
  virtual const char*      getId()  { return WIFI_MODULE; }
  virtual const char*      getName()  { return Messages::TITLE_WIFI_MODULE; }
  // Module Web interface
  virtual const String     getModuleWebpage();
  virtual const String     getStatusWebpage();
  // A generic getData/setData interface
  virtual const String     getString( const String& key );
  virtual ResultData       setString( const String& key, const String& value );

  virtual void             reinitModule();

  void                     begin();
  bool                     getDebugLog()  { return flags.debug_log_enabled; }
  uint8_t                  getStatus();
  void                     setSleepMode( bool enabled );
  void                     setDebugLog( bool enabled )  { flags.debug_log_enabled = enabled; }

protected:
  virtual bool             handleCommand( const String& cmd, const String& args );
  virtual ResultData       handleOption( const String& key, const String& value, Options::Action action );
  virtual void             resolveTemplateKey( const String& key, String& out );

private:
  void                     checkConnection();
  Config::WifiConfigMethod getConfigMethodOption();
  String                   getIPAddressOption( const char* key, const char* def );
  String                   getSSID( const uint8_t index );
  String                   getPassword( const uint8_t index );
  void                     reconfigure( const Config::WifiConfigMethod method );
  void                     reconnect( ReconnectMode mode );

  static Config::WifiConfigMethod toConfigMethod( const String& value );
  static String toString( const Config::WifiConfigMethod method );
};
