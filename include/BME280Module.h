#pragma once
#include "SparkFunBME280.h"
#include "Module.h"

/**
 * BME280 is a temperature + humidity + pressure sensor.
 */
class BME280Module : public Module {
private:
  enum State {SLEEP, WAKEUP};

  BME280   sensor;
  bool     sensorDetected;
  State    state;
  uint32_t lastMeasureTimestamp;
  uint32_t pollIntervalMs;
  // temperature related
  String   temperature;
  float    previousTemperature;
  float    temperatureDelta;
  // humidity related
  String   humidity;
  float    previousHumidity;
  float    humidityDelta;

public:
  BME280Module();
  virtual ~BME280Module();
  virtual void             tick_100mS( uint8_t phase );
  // Module identification
  virtual const char*      getId()    { return BME280_MODULE; }
  virtual const char*      getName()  { return Messages::TITLE_BME280_MODULE; }
  // Module Web interface
  virtual const String     getModuleWebpage();
  virtual const String     getStatusWebpage();
  // A generic getData/setData interface
  virtual const String     getString( const String& key );
  virtual ResultData       setString( const String& key, const String& value );

protected:
  virtual bool             handleCommand( const String& cmd, const String& args );
  virtual ResultData       handleOption( const String& key, const String& value, Options::Action action );
  virtual void             resolveTemplateKey( const String& key, String& out );

private:
  String                   getDeltaTOption();
  String                   getDeltaHOption();
  uint16_t                 getPollOption();
  Config::TemperatureScale getScaleOption();
  String                   toJsonString();

  static Config::TemperatureScale toScale( const String& value );
  static String            toString( const Config::TemperatureScale scale );
};