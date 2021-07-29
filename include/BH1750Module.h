#pragma once
#include "Module.h"
#include "BH1750.h"

/**
 * BH1750 is a light sensor.
 */
class BH1750Module : public Module {
private:
  static constexpr const char* const DELTA_OPTION_KEY    = "Delta";
  static constexpr const char* const POLL_OPTION_KEY     = "Pin";

  BH1750   sensor;
  bool     sensorDetected;

  uint32_t lastMeasureTimestamp;
  uint32_t pollIntervalMs;

  String   lux;
  float    valueDelta;
  float    previousValue;

public:
  BH1750Module();
  virtual ~BH1750Module();
  virtual void             tick_100mS( uint8_t phase );
  // Module identification
  virtual const char*      getId()    { return BH1750_MODULE; }
  virtual const char*      getName()  { return Messages::TITLE_BH1750_MODULE; }
  // Module Web interface
  virtual const String     getModuleWebpage();
  virtual const String     getStatusWebpage();
  // A generic getData/setData interface
  virtual const String     getString( const String& key );
  virtual ResultData       setString( const String& key, const String& value );

protected:
  virtual ResultData       handleOption( const String& key, const String& value, Options::Action action );
  virtual void             resolveTemplateKey( const String& key, String& out );

private:
  uint16_t                 getPollOption();
  String                   getValueDeltaOption();
};