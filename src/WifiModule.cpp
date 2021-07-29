#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "Events.h"
#include "Messages.h"
#include "Options.h"
#include "str_switch.h"
#include "Utils.h"
#include "WifiModule.h"

/* Public */

WifiModule::WifiModule() {
  properties.tick_100mS_required = true;
  properties.has_module_webpage = true;
  properties.has_status_webpage = true;
  flags.data = 0;
  connectionStatus = 0;
}

WifiModule::~WifiModule() {
  // do nothing
}

/**
 * Checks the WiFi state once per second.
 * When connection is lost, performs auto reconnect.
 */
void WifiModule::tick_100mS( uint8_t phase ) {
  if( phase == 6 ) {
    // Check the reconfig case
    if( flags.invoke_reconfig ) {
      if( reconfigDelayCounter > 0 ) {
        reconfigDelayCounter -= 1;
      } else {
        flags.invoke_reconfig = false;
        reconfigure( getConfigMethodOption() );
      }
    }
    // Check the restart case
    else if( flags.invoke_restart ) {
      if( reconfigDelayCounter > 0 ) {
        reconfigDelayCounter -= 1;
      } else {
        flags.invoke_restart = false;
        reconfigure( Config::WifiConfigMethod::WIFI_RESTART );
      }
    }
    // Check connection
    else if( flags.web_manager_started ) {
      if( WiFi.softAPgetStationNum() > 0 ) {
        checkConnectionDelayCounter = Config::WIFI_INACTIVITY_CHECK_TIME;
      } else if( --checkConnectionDelayCounter <= 0 ) {
        reconfigure( Config::WifiConfigMethod::WIFI_RETRY );
      }
    } else {
      if( --checkConnectionDelayCounter == 0 ) {
        checkConnection();
      }
    }
  }
}

/* Module Web interface */

const String WifiModule::getModuleWebpage() {
  return makeWebpage( "/module_wifi_client.html" );
}

const String WifiModule::getStatusWebpage() {
  return makeWebpage( "/status_wifi_client.html" );
}

/* A generic getData/setData interface */

const String WifiModule::getString( const String& key ) {
  SWITCH( key.c_str() ) {
    CASE( Config::KEY_EXPORT_CONFIGURATION ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["hostname"]     = getStringOption( "Hostname", Config::WIFI_HOSTNAME );
      json["ssid1"]        = getStringOption( "SSID1", Config::WIFI_SSID1 );
      json["pwd1"]         = getStringOption( "Pwd1", Config::WIFI_PASSWORD1 );
      json["ssid2"]        = getStringOption( "SSID2", Config::WIFI_SSID2 );
      json["pwd2"]         = getStringOption( "Pwd2", Config::WIFI_PASSWORD2 );
      json["cfgmethod"]    = toString( getConfigMethodOption() );
      json["ipaddr"]       = getIPAddressOption( "IpAddr", Config::WIFI_IP_ADDRESS );
      json["ipmask"]       = getIPAddressOption( "NetMask", Config::WIFI_SUBNET_MASK );
      json["ipgate"]       = getIPAddressOption( "Gateway", Config::WIFI_GATEWAY );
      json["ipdns"]        = getIPAddressOption( "Dns", Config::WIFI_DNS );
      return json.as<String>();
    }
    DEFAULT_CASE:
      return Module::getString( key );
  }
}

ResultData WifiModule::setString( const String& key, const String& value ) {
  SWITCH( key.c_str() ) {
    CASE( Config::KEY_IMPORT_CONFIGURATION ):
      return handleConfigImport( value );
    DEFAULT_CASE:
      return Module::setString( key, value );
  }
}

void WifiModule::begin() {
  connectionStatus = 0;
  checkConnectionDelayCounter = 1;                    // 1 means that during the next 1sec loop the checkConnection will be triggered

  const String ssid1 = getStringOption( "SSID1", Config::WIFI_SSID1 );
  const String ssid2 = getStringOption( "SSID2", Config::WIFI_SSID2 );
  connectionRetries = ssid1.length() || ssid2.length() ? Config::WIFI_CONNECT_RETRIES : 0;

  Bus.notify( (ConnectivityEvent) {ConnectivityEvent::TYPE_WIFI, false} );
  WiFi.persistent( false );                           // Solve possible wifi init errors
                                                      // persistent(false) means "not store WiFi config in SDK flash area"
}

uint8_t WifiModule::getStatus() {
  return WiFi.status();
}

void WifiModule::setSleepMode( bool enabled ) {
  WiFi.setSleep( enabled );
}

/* Protected */

bool WifiModule::handleCommand( const String& cmd, const String& args ) {
  SWITCH( cmd.c_str() ) {
    // ==========================================
    // Enables or disables the module debug log.
    // debug 0/1 - Disable/enable WiFi debug log.
    // Returns the actual value of debug log parameter.
    CASE( "debug" ):
      if( args.length() > 0 ) {
        setDebugLog( Utils::toBool( args.c_str() ));
      }
      handleCommandResults( cmd, args, String( getDebugLog() ));
      return true;
    // ==========================================
    CASE( "reconnect" ):
      reinitModule();
      handleCommandResults( cmd, args, Messages::OK );
      return true;
    // ==========================================
    CASE( "manager" ):
      reconfigure( Config::WifiConfigMethod::WIFI_MANAGER );
      handleCommandResults( cmd, args, Messages::OK );
      return true;
    // ==========================================
    DEFAULT_CASE:
      return false;
  }
}

ResultData WifiModule::handleOption( const String& key, const String& value, Options::Action action ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    CASE( "hostname" ):
      return handleStringOption( "Hostname", value, action, {NOT_EMPTY, IMPORTANT} );
    // ==========================================
    CASE( "ssid1" ):
      return handleStringOption( "SSID1", value, action, {OPTIONAL, IMPORTANT} );
    // ==========================================
    CASE( "pwd1" ):
      return handleStringOption( "Pwd1", value, action, {OPTIONAL, IMPORTANT} );
    // ==========================================
    CASE( "ssid2" ):
      return handleStringOption( "SSID2", value, action, {OPTIONAL, IMPORTANT} );
    // ==========================================
    CASE( "pwd2" ):
      return handleStringOption( "Pwd2", value, action, {OPTIONAL, IMPORTANT} );
    // ==========================================
    CASE( "cfgmethod" ): {
      auto method = toConfigMethod( value );
      if( method == Config::WIFI_INVALID && action != Options::READ ) {
        return INVALID_VALUE;
      }
      ResultData rc = handleByteOption( "Config", String(method), action, false );
      rc.details = toString( getConfigMethodOption() );
      return rc;
    }
    // ==========================================
    CASE( "ipaddr" ):
      return handleIpAddressOption( "IpAddr", value, action, {NOT_EMPTY, false} );
    // ==========================================
    CASE( "ipmask" ):
      return handleIpAddressOption( "NetMask", value, action, {NOT_EMPTY, false} );
    // ==========================================
    CASE( "ipgate" ):
      return handleIpAddressOption( "Gateway", value, action, {NOT_EMPTY, false} );
    // ==========================================
    CASE( "ipdns" ):
      return handleIpAddressOption( "Dns", value, action, {NOT_EMPTY, false} );
    // ==========================================
    DEFAULT_CASE:
      return UNKNOWN_OPTION;
  }
}

void WifiModule::reinitModule() {
  // Ignore the request to reconfig the Wifi module in the web manager mode because in a result
  // the module will be switched back to usual wifi client mode.
  if( !State.webManagerMode() ) {
    reconfigDelayCounter = 2;
    flags.invoke_reconfig = true;
  }
}

void WifiModule::resolveTemplateKey( const String& key, String& out ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    // Module template parameters
    CASE( "SSID1" ):      out += getStringOption( "SSID1", Config::WIFI_SSID1 );                    break;
    CASE( "SSID2" ):      out += getStringOption( "SSID2", Config::WIFI_SSID2 );                    break;
    CASE( "PWD1" ):       out += getStringOption( "Pwd1", Config::WIFI_PASSWORD1 );                 break;
    CASE( "PWD2" ):       out += getStringOption( "Pwd2", Config::WIFI_PASSWORD2 );                 break;
    CASE( "WF_RST" ):     out += getConfigMethodOption() == Config::WIFI_RESTART ? "selected" : ""; break;
    CASE( "WF_MGR" ):     out += getConfigMethodOption() == Config::WIFI_MANAGER ? "selected" : ""; break;
    CASE( "WF_RETRY" ):   out += getConfigMethodOption() == Config::WIFI_RETRY ? "selected" : "";   break;
    CASE( "WF_WAIT" ):    out += getConfigMethodOption() == Config::WIFI_WAIT ? "selected" : "";    break;
    CASE( "IP_ADDR" ):    out += getIPAddressOption( "IpAddr", Config::WIFI_IP_ADDRESS );           break;
    CASE( "IP_MASK" ):    out += getIPAddressOption( "NetMask", Config::WIFI_SUBNET_MASK );         break;
    CASE( "IP_GATE" ):    out += getIPAddressOption( "Gateway", Config::WIFI_GATEWAY );             break;
    CASE( "IP_DNS" ):     out += getIPAddressOption( "Dns", Config::WIFI_DNS );                     break;
    CASE( "WF_THOST" ):   out += getStringOption( "Hostname", Config::WIFI_HOSTNAME );              break;
    CASE( "WF_VHOST" ):   out += getMacroOption( "Hostname", Config::WIFI_HOSTNAME );               break;
    CASE( "Title" ):      out += Utils::formatModuleSettingsTitle( getId(), getName() );            break;
    // ==========================================
    // Status template parameters
    CASE( "SSID" ):       out += WiFi.SSID();                  break;
    CASE( "MAC" ):        out += WiFi.macAddress();            break;
    CASE( "IP" ):         out += WiFi.localIP().toString();    break;
    CASE( "DNS" ):        out += WiFi.dnsIP().toString();      break;
    CASE( "GATE" ):       out += WiFi.gatewayIP().toString();  break;
  }
}

/* Private */

Config::WifiConfigMethod WifiModule::getConfigMethodOption() {
  switch( getByteOption( "Config", Config::WIFI_CONFIG_METHOD )) {
    case 0:   return Config::WIFI_RESTART;
    case 1:   return Config::WIFI_MANAGER;
    case 2:   return Config::WIFI_RETRY;
    case 3:   return Config::WIFI_WAIT;
    default:  return Config::WIFI_INVALID;
  }
}

String WifiModule::getIPAddressOption( const char* key, const char* def ) {
  const uint32_t ip = getLongOption( key, Utils::parseIpString( def ));
  return IPAddress( ip ).toString();
}

String WifiModule::getSSID( const uint8_t index ) {
  return index == 1
    ? getStringOption( "SSID2", Config::WIFI_SSID2 )
    : getStringOption( "SSID1", Config::WIFI_SSID1 );
}

String WifiModule::getPassword( const uint8_t index ) {
  return index == 1
    ? getStringOption( "Pwd2", Config::WIFI_PASSWORD2 )
    : getStringOption( "Pwd1", Config::WIFI_PASSWORD1 );
}

/**
 * Checks connection to access point every Config::WIFI_CHECK_TIME second(s).
 * When connection lost, performs auto reconnect.
 */
void WifiModule::checkConnection() {
  uint8_t _st = WiFi.status();
  if( flags.debug_log_enabled ) {
    Log.verbose( "WiFi connection status %d (%d)" CR, _st, connectionRetries );
  }

  if( (_st == WL_CONNECTED) && (static_cast<uint32_t>(WiFi.localIP()) != 0) ) {
    connectionRetries = Config::WIFI_CONNECT_RETRIES;
    checkConnectionDelayCounter = Config::WIFI_CHECK_TIME;
    if( connectionStatus != WL_CONNECTED ) {
      // Update log and state
      Log.notice( "WiFi Connected (%s)" CR, WiFi.localIP().toString().c_str() );
      connectionStatus = WL_CONNECTED;
      Bus.notify( (ConnectivityEvent) {ConnectivityEvent::TYPE_WIFI, true} );
    }
  } else {
    // Disconnect is detected, update the state once
    if( connectionStatus == WL_CONNECTED ) {
      Log.notice( "WiFi Disconnected" CR );
      Bus.notify( (ConnectivityEvent) {ConnectivityEvent::TYPE_WIFI, false} );
    }

    connectionStatus = _st;
    switch( connectionStatus ) {
      case WL_CONNECTED:
        Log.warning( "WiFi Connect failed as no IP address received" CR );
        connectionStatus = 0;
        break;
      case WL_NO_SSID_AVAIL:
        Log.warning( "WiFi Connect failed as AP cannot be reached" CR );
        break;
      case WL_CONNECT_FAILED:
        Log.warning( "WiFi Connect failed with AP incorrect password" CR );
        break;
      case WL_DISCONNECTED:
        checkConnectionDelayCounter = 2;
        break;
      default:     // WL_IDLE_STATUS
        break;
    }
    if( connectionRetries ) {
      if( (getConfigMethodOption() != Config::WIFI_WAIT) && (connectionRetries % 2 == 0) ) {
        reconnect( CURRENT_AP );       // Select alternate SSID
      } else {
        reconnect( TOGGLE_AP);      // Select default SSID
      }

      checkConnectionDelayCounter = Config::WIFI_CONNECT_DELAY;
      connectionRetries--;
    } else {
      // no more retries
      reconfigure( getConfigMethodOption() );
    }
  }
}

void WifiModule::reconfigure( const Config::WifiConfigMethod method ) {
  checkConnectionDelayCounter = 1;
  connectionRetries = Config::WIFI_CONNECT_RETRIES;
  WiFi.disconnect();        // Solve possible Wifi hangs

  if( method == Config::WIFI_RESTART ) {
    Bus.notify( (SystemEvent) {SystemEvent::TYPE_PENDING_RESTART} );
  }
  else if( method == Config::WIFI_MANAGER ) {
    // Notify that WiFi is disconnected.
    Bus.notify( (ConnectivityEvent) {ConnectivityEvent::TYPE_WIFI, false} );
    // Activate the wifi manager mode. Tune the connection check timer to 5 minutes.
    if( !WiFi.mode( WIFI_AP )) {
      Log.error( "WiFi Failed to switch to AP mode" CR );
      return;
    }
    if( !WiFi.softAP( Config::WEB_SERVER_AP_SSID, Config::WEB_SERVER_AP_PASSWORD )) {
      Log.error( "WiFi Failed to setup AP mode" CR );
      return;
    }
    flags.web_manager_started = true;
    checkConnectionDelayCounter = Config::WIFI_INACTIVITY_CHECK_TIME;
    // Notify that web manager is activated.
    Log.notice( "WiFi Web manager is started as %s on %s" CR, Config::WEB_SERVER_AP_SSID, "192.168.4.1" );
    Bus.notify( (ConnectivityEvent) {ConnectivityEvent::TYPE_ACCESS_POINT, true} );
    delay( 100 );
  }
}

/**
 * Starts the connection attempt to the specified access point.
 */
void WifiModule::reconnect( ReconnectMode mode ) {
  WiFi.persistent( false );         // Solve possible wifi init errors
  WiFi.disconnect( true );          // Delete SDK wifi config
  delay( 200 );
  WiFi.mode( WIFI_STA );            // Disable AP mode
  if( !WiFi.getAutoConnect() ) {
    WiFi.setAutoConnect( true );
  }

  switch( mode ) {
    case AP1:
      active_ap = 0;
      break;
    case AP2:
      active_ap = 1;
      break;
    case TOGGLE_AP:
      active_ap = active_ap == 0 ? 1 : 0;
      break;
    case CURRENT_AP:
      break;
  }

  // Skip empty SSID
  String ssid = getSSID( active_ap );
  if( ssid.length() == 0 ) {
    active_ap = active_ap == 0 ? 1 : 0;
    ssid = getSSID( active_ap );
  }

  if( ssid.length() > 0 ) {
    // Set static IP if IP address is filled
    if( getLongOption( "IpAddr", Utils::parseIpString( Config::WIFI_IP_ADDRESS )) != 0 ) {
      WiFi.config( getLongOption( "IpAddr", Utils::parseIpString( Config::WIFI_IP_ADDRESS )),
                   getLongOption( "Gateway", Utils::parseIpString( Config::WIFI_GATEWAY )),
                   getLongOption( "NetMask", Utils::parseIpString( Config::WIFI_SUBNET_MASK )),
                   getLongOption( "Dns", Utils::parseIpString( Config::WIFI_DNS )));
    }
    const String hostname = getMacroOption( "Hostname", Config::WIFI_HOSTNAME );
    WiFi.setHostname( hostname.c_str() );
    WiFi.begin( ssid.c_str(), getPassword( active_ap ).c_str() );
    Log.notice( "WiFi Connecting to AP #%d '%s' as %s (%d)..." CR, active_ap+1, ssid.c_str(), hostname.c_str(), connectionRetries );
  }
}

/* Private static */

Config::WifiConfigMethod WifiModule::toConfigMethod( const String& value ) {
  SWITCH( value.c_str() ) {
    CASE( "Restart" ):  return Config::WIFI_RESTART;
    CASE( "Manager" ):  return Config::WIFI_MANAGER;
    CASE( "Retry" ):    return Config::WIFI_RETRY;
    CASE( "Wait" ):     return Config::WIFI_WAIT;
    DEFAULT_CASE:       return Config::WIFI_INVALID;
  }
}

String WifiModule::toString( const Config::WifiConfigMethod method ) {
  switch( method ) {
    case Config::WIFI_RESTART:    return "Restart";
    case Config::WIFI_MANAGER:    return "Manager";
    case Config::WIFI_RETRY:      return "Retry";
    case Config::WIFI_WAIT:       return "Wait";
    default:                      return "Invalid value";
  }
}