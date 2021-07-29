#pragma once
#include "Module.h"

/**
 * AM312 is a pyroelectric motion sensor.
 */
class AM312Module : public Module {
private:
  static constexpr const char* const PIN_OPTION_KEY     = "Pin";
  static constexpr const char* const HOLD_OPTION_KEY    = "Hold";

  uint8_t  sensorPin;
  bool     sensorValue;
  bool     statusValue = false;
  uint32_t lastMotionTimestamp;
  uint32_t holdValueMs;

public:
  AM312Module();
  virtual ~AM312Module();
  virtual void             tick_100mS( uint8_t phase );
  // Module identification
  virtual const char*      getId()    { return AM312_PIR_MODULE; }
  virtual const char*      getName()  { return Messages::TITLE_AM312_MODULE; }
  // Module Web interface
  virtual const String     getModuleWebpage();
  virtual const String     getStatusWebpage();
  // A generic getData/setData interface
  virtual const String     getString( const String& key );
  virtual ResultData       setString( const String& key, const String& value );

protected:
  virtual ResultData       handleOption( const String& key, const String& value, Options::Action action );
  virtual void             resolveTemplateKey( const String& key, String& out );
};