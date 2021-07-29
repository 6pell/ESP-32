#pragma once

#include <Arduino.h>
#include <Ticker.h>
#include "Module.h"

class RtcTimeModule : public Module {
private:

  const uint16_t RECONFIGURE_TIMEOUT = 60 * 10;     // NTP sync timeout, seconds

  Ticker     rtcTicker;
  bool       isMidnight;
  tm         timeInfo;
  time_t     uptime = 0;
  time_t     local_time = 0;
  time_t     restart_time = 0;
  uint16_t   reconfigureTimeout = 0;
  int        eventBusToken;

public:
  RtcTimeModule();
  virtual ~RtcTimeModule();
  // Module identification
  virtual const char*   getId()  { return RTC_MODULE; }
  virtual const char*   getName()  { return Messages::TITLE_RTC_MODULE; }
  // Module Web interface
  virtual const String  getModuleWebpage();
  virtual const String  getStatusWebpage();
  // A generic getData/setData interface
  virtual const String  getString( const String& key );
  virtual ResultData    setString( const String& key, const String& value );

  virtual void          reinitModule() { reconfigureNtp(); }

  const String          getLocalTimeString();
  const String          getLocalTimeIsoString();
  const String          getUptimeString();
  const uint32_t        getUptimeSeconds()  { return uptime; }
  bool                  isMidnightNow();
  bool                  reconfigureNtp();

protected:
  virtual bool          handleCommand( const String& cmd, const String& args );
  virtual ResultData    handleOption( const String& key, const String& value, Options::Action action );
  virtual void          resolveTemplateKey( const String& key, String& out );

private:
  void                  handleTickEverySecond( void );
  static String         getIsoDateTime( time_t value );
  static void           tickerCallback( RtcTimeModule* pThis );
};
