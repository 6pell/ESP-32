#pragma once
#include "Module.h"
#include "mongoose.h"
#include "OpenWeatherData.h"

class OpenWeatherMapModule : public Module {
private:
  const char* WEATHER_API_HOST = "api.openweathermap.org";

  struct RequestData {
    String url;
  };

  static OpenWeatherMapModule* INSTANCE;
  mg_mgr   manager;
  bool     updateStarted = false;
  uint32_t lastRequestTimestamp;
  uint32_t pollIntervalMs;
  OpenWeather::WeatherData weatherData;

public:
  OpenWeatherMapModule();
  virtual ~OpenWeatherMapModule();
  virtual void                loop();
  virtual void                tick_100mS( uint8_t phase );
  // Module identification
  virtual const char*         getId()    { return OPEN_WEATHER_MAP_MODULE; }
  virtual const char*         getName()  { return Messages::TITLE_OPEN_WEATHER_MAP_MODULE; }
  // Module Web interface
  virtual const String        getModuleWebpage();
  virtual const String        getStatusWebpage();

protected:
  virtual bool                handleCommand( const String& cmd, const String& args );
  virtual ResultData          handleOption( const String& key, const String& value, Options::Action action );
  virtual void                resolveTemplateKey( const String& key, String& out );

private:
  String                      formatTemperature( float value );
  String                      getAppIDOption();
  uint16_t                    getAutoUpdateOption();
  String                      getLatitudeOption();
  String                      getLongitudeOption();
  String                      getExcludeOption();
  String                      getLanguageCodeOption();
  Config::WeatherUnits        getUnitsOption();
  bool                        hasWeatherData();
  void                        httpEventHandler( mg_connection* nc, int ev, void* ev_data );
  void                        updateWeather();
  void                        updateWeatherCleanup();

  static String               formatTime( long time );
  static String               toString( const Config::WeatherUnits units );
  static Config::WeatherUnits toUnits( const String& value );
};