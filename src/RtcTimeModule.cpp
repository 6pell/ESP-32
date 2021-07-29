#include <ArduinoLog.h>
#include "Events.h"
#include "Messages.h"
#include "Options.h"
#include "RtcTimeModule.h"
#include "str_switch.h"
#include "Utils.h"

// Useful links
// https://en.cppreference.com/w/cpp/header/ctime
// https://github.com/espressif/esp-idf/tree/master/examples/protocols/sntp
// http://cppstudio.com/post/587/
// http://www.lucadentella.it/en/2017/05/11/esp32-17-sntp/

/* Public */

RtcTimeModule::RtcTimeModule() {
  properties.has_module_webpage = true;
  properties.has_status_webpage = true;

  time_t now;
  time( &now );
  localtime_r( &now, &timeInfo );

  // Lambda expression, an EventBus callback handler.
  eventBusToken = Bus.listen<ConnectivityEvent>( [this](const ConnectivityEvent& event ) {
    if( event.type == ConnectivityEvent::TYPE_WIFI ) {
      if( event.connected ) {
        reconfigureNtp();
      }
    }
  });
  rtcTicker.attach( 1, tickerCallback, this );
}

RtcTimeModule::~RtcTimeModule() {
  Bus.unlisten<ConnectivityEvent>( eventBusToken );
  rtcTicker.detach();
}

const String RtcTimeModule::getModuleWebpage() {
  return makeWebpage( "/module_rtc.html" );
}

const String RtcTimeModule::getStatusWebpage() {
  return makeWebpage( "/status_rtc.html" );
}

const String RtcTimeModule::getString( const String& key ) {
  SWITCH( key.c_str() ) {
    CASE( Config::KEY_EXPORT_CONFIGURATION ): {
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["timezone"]  = getStringOption( "TZone", Config::RTC_TIMEZONE );
      json["ntp1"]      = getStringOption( "Ntp1", Config::RTC_NTP_SERVER1 );
      json["ntp2"]      = getStringOption( "Ntp2", Config::RTC_NTP_SERVER2 );
      json["ntp3"]      = getStringOption( "Ntp3", Config::RTC_NTP_SERVER3 );
      json["gmtoffset"] = getLongOption( "Gmt", Config::RTC_GMT_OFFSET );
      json["dstoffset"] = getShortOption( "Dst", Config::RTC_DST_OFFSET );
      return json.as<String>();
    }
    DEFAULT_CASE:
      return Module::getString( key );
  }
}

ResultData RtcTimeModule::setString( const String& key, const String& value ) {
  SWITCH( key.c_str() ) {
    CASE( Config::KEY_IMPORT_CONFIGURATION ):
      return handleConfigImport( value );
    DEFAULT_CASE:
      return Module::setString( key, value );
  }
}

const String RtcTimeModule::getLocalTimeString() {
  struct tm tmp;
  localtime_r( &local_time, &tmp );

  char buf[64];
  strftime( buf, 64, "%a, %b %d %Y %H:%M:%S", &tmp );
  //snprintf_P( stime, sizeof(stime), sntp_get_real_time( time ));
  return String( buf );  // Thu Nov 01 11:41:02 2018
}

const String RtcTimeModule::getLocalTimeIsoString() {
  return getIsoDateTime( local_time );
}

const String RtcTimeModule::getUptimeString() {
  char dt[16];
  uint8_t second;
  uint8_t minute;
  uint8_t hour;
  unsigned long days;

  time_t t = restart_time > 0 && local_time > restart_time
   ? local_time - restart_time
   : uptime;

  second = t % 60;
  t /= 60;                // now it is minutes
  minute = t % 60;
  t /= 60;                // now it is hours
  hour = t % 24;
  t /= 24;                // now it is days
  days = t;

  // "P128DT14H35M44S" - ISO8601:2004 - https://en.wikipedia.org/wiki/ISO_8601 Durations
  //  snprintf_P(dt, sizeof(dt), PSTR("P%dDT%02dH%02dM%02dS"), ut.days, ut.hour, ut.minute, ut.second);
  // "128 14:35:44" - OpenVMS
  // "128T14:35:44" - Tasmota
  snprintf( dt, sizeof(dt), "%luT%02d:%02d:%02d", days, hour, minute, second );
  return String( dt );  // 128T14:35:44
}

bool RtcTimeModule::isMidnightNow() {
  bool mnflg = isMidnight;
  if( mnflg ) {
    isMidnight = false;
  }
  return mnflg;
}

bool RtcTimeModule::reconfigureNtp() {
  if( !State.wifiConnected() ) {
    return false;
  }

  const String tz = getStringOption( "TZone", Config::RTC_TIMEZONE );
  const String ntp1 = getStringOption( "Ntp1", Config::RTC_NTP_SERVER1 );
  const String ntp2 = getStringOption( "Ntp2", Config::RTC_NTP_SERVER2 );
  const String ntp3 = getStringOption( "Ntp3", Config::RTC_NTP_SERVER3 );

  if( tz.length() > 0 ) {
    configTzTime( tz.c_str(), ntp1.c_str(), ntp2.c_str(), ntp3.c_str() );
  } else {
    const long gmt = getLongOption( "Gmt", Config::RTC_GMT_OFFSET );
    const int dst = getShortOption( "Dst", Config::RTC_DST_OFFSET );
    configTime( gmt, dst, ntp1.c_str(), ntp2.c_str(), ntp3.c_str() );
  }
  return true;
}

/* Protected */

bool RtcTimeModule::handleCommand( const String& cmd, const String& args ) {
  SWITCH( cmd.c_str() ) {
    // ==========================================
    CASE( "reconfig" ): {
      reconfigureNtp();
      handleCommandResults( cmd, args, Messages::OK );
      return true;
    }
    // ==========================================
    DEFAULT_CASE:
      return false;
  }
}

ResultData RtcTimeModule::handleOption( const String& key, const String& value, Options::Action action ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    CASE( "timezone" ):
      return handleStringOption( "TZone", value, action, {NOT_EMPTY, IMPORTANT} );
    // ==========================================
    CASE( "ntp1" ):
      return handleStringOption( "Ntp1", value, action, {NOT_EMPTY, IMPORTANT} );
    // ==========================================
    CASE( "ntp2" ):
      return handleStringOption( "Ntp2", value, action, {OPTIONAL, IMPORTANT} );
    // ==========================================
    CASE( "ntp3" ):
      return handleStringOption( "Ntp3", value, action, {OPTIONAL, IMPORTANT} );
    // ==========================================
    CASE( "gmtoffset" ):
      return handleLongOption( "Gmt", value, action, true );
    // ==========================================
    CASE( "dstoffset" ):
      return handleShortOption( "Dst", value, action, true );
    // ==========================================
    DEFAULT_CASE:
      return UNKNOWN_OPTION;
  }
}

void RtcTimeModule::resolveTemplateKey( const String& key, String& out ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    // Module template parameters
    CASE( "NTP1" ):       out += getStringOption( "Ntp1", Config::RTC_NTP_SERVER1 );      break;
    CASE( "NTP2" ):       out += getStringOption( "Ntp2", Config::RTC_NTP_SERVER2 );      break;
    CASE( "NTP3" ):       out += getStringOption( "Ntp3", Config::RTC_NTP_SERVER3 );      break;
    CASE( "NTP_TZONE" ):  out += getStringOption( "TZone", Config::RTC_TIMEZONE );        break;
    CASE( "NTP_GMT" ):    out += getLongOption( "Gmt", Config::RTC_GMT_OFFSET );          break;
    CASE( "NTP_DST" ):    out += getShortOption( "Dst", Config::RTC_DST_OFFSET );         break;
    CASE( "TITLE" ):      out += Utils::formatModuleSettingsTitle( getId(), getName() );  break;
    // ==========================================
    // Status template parameters
    CASE( "TIME" ):       out += getLocalTimeString();                                    break;
    CASE( "UPTIME" ):     out += getUptimeString();                                       break;
  }
}

/* Private */

void RtcTimeModule::handleTickEverySecond( void ) {
  uptime++;

  time_t now;
  time( &now );
  localtime_r( &now, &timeInfo );
  //Log.notice( F("RTC LocalTime %s" CR), asctime( &timeInfo ));

  if( timeInfo.tm_year > (2016-1900) ) {
    // Synchronized...
    local_time = now;
    reconfigureTimeout = 0;
    // Save the first NTP time as a device restart time.
    if( restart_time == 0 ) {
      restart_time = now - uptime;
    }
    // Set flag every midnight.
    if( !timeInfo.tm_hour && !timeInfo.tm_min && !timeInfo.tm_sec ) {
      isMidnight= true;
    }
    // Send a status update event to EventBus every minute.
    if( timeInfo.tm_sec == 0 ) {
      Bus.notify( (StatusChangedEvent) { this, getLocalTimeIsoString() });
    }
  } else {
    // Not synchronized...
    if( State.wifiConnected() && ++reconfigureTimeout >= RECONFIGURE_TIMEOUT ) {
      reconfigureTimeout = 0;
      reconfigureNtp();
    }
  }
}

/* Private static */

String RtcTimeModule::getIsoDateTime( time_t value ) {
  // "2017-03-07T11:08:02" - ISO8601:2004
  char dt[20];
  struct tm tmpTime;

  localtime_r( &value, &tmpTime );
  snprintf( dt, sizeof(dt), "%04d-%02d-%02dT%02d:%02d:%02d",
    tmpTime.tm_year + 1900, tmpTime.tm_mon + 1, tmpTime.tm_mday, tmpTime.tm_hour, tmpTime.tm_min, tmpTime.tm_sec );
  return String(dt);  // 2017-03-07T11:08:02
}

void RtcTimeModule::tickerCallback( RtcTimeModule* pThis ) {
  pThis->handleTickEverySecond();
}