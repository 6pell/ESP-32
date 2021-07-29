#include "Events.h"
#include "OpenWeatherMapModule.h"
#include "str_switch.h"
#include "Utils.h"

OpenWeatherMapModule* OpenWeatherMapModule::INSTANCE = nullptr;

OpenWeatherMapModule::OpenWeatherMapModule() {
  properties.has_module_webpage = true;
  properties.has_status_webpage = true;
  properties.loop_required = true;
  properties.tick_100mS_required = true;
  INSTANCE = this;

  pollIntervalMs = getAutoUpdateOption() * 60 * 1000;
}

OpenWeatherMapModule::~OpenWeatherMapModule() {
  updateWeatherCleanup();
  INSTANCE = nullptr;
}

void OpenWeatherMapModule::loop() {
  if( updateStarted ) {
    mg_mgr_poll( &manager, 0 );
  }
}

void OpenWeatherMapModule::tick_100mS( uint8_t phase ) {
  if( phase == 0 && pollIntervalMs > 0 && State.wifiConnected() ) {
    const unsigned long now = millis();
    const unsigned long delta = now - lastRequestTimestamp;
    if( delta > pollIntervalMs ) {
      lastRequestTimestamp = now;
      updateWeather();
    }
  }
}

const String OpenWeatherMapModule::getModuleWebpage() {
  return makeWebpage( "/module_open_weather.html" );
}

const String OpenWeatherMapModule::getStatusWebpage() {
  return makeWebpage( "/status_open_weather.html" );
}

/* Protected */

bool OpenWeatherMapModule::handleCommand( const String& cmd, const String& args ) {
  SWITCH( cmd.c_str() ) {
    // ==========================================
    CASE( "update" ):
      updateWeather();
      handleCommandResults( cmd, args, Messages::OK );
      return true;
    // Unknown command
    DEFAULT_CASE:
      return false;
  }
}

ResultData OpenWeatherMapModule::handleOption( const String& key, const String& value, Options::Action action ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    CASE( "appid" ): {
      return handleStringOption( "AppID", value, action, {NOT_EMPTY, IMPORTANT} );
    }
    // ==========================================
    CASE( "autoupd" ): {
      ResultData rc = handleShortOption( "AutoUpd", value, action, false );
      if( rc.code == RC_OK && action == Options::SAVE ) {
        pollIntervalMs = getAutoUpdateOption() * 60 * 1000;
      }
      return rc;
    }
    // ==========================================
    CASE( "lat" ): {
      return handleStringOption( "Lat", value, action, {NOT_EMPTY, IMPORTANT} );
    }
    // ==========================================
    CASE( "lon" ): {
      return handleStringOption( "Lon", value, action, {NOT_EMPTY, IMPORTANT} );
    }
    // ==========================================
    CASE( "exclude" ): {
      return handleStringOption( "Exclude", value, action, {OPTIONAL, IMPORTANT} );
    }
    // ==========================================
    CASE( "lang" ): {
      return handleStringOption( "LangCode", value, action, {OPTIONAL, IMPORTANT} );
    }
    // ==========================================
    CASE( "units" ): {
      auto units = toUnits( value );
      if( units == Config::UNKNOWN_UNITS && action != Options::READ ) {
        return INVALID_VALUE;
      }
      ResultData rc = handleByteOption( "Units", String(units), action, IMPORTANT );
      rc.details = toString( getUnitsOption() );
      return rc;
    }
    DEFAULT_CASE:
      return UNKNOWN_OPTION;
  }
}

void OpenWeatherMapModule::resolveTemplateKey( const String& key, String& out ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    // Module template parameters
    CASE( "MTITLE" ):    out += Utils::formatModuleSettingsTitle( getId(), getName() );   break;
    CASE( "APP_ID" ):    out += getAppIDOption();                                         break;
    CASE( "UPD" ):       out += getAutoUpdateOption();                                    break;
    CASE( "LAT" ):       out += getLatitudeOption();                                      break;
    CASE( "LON" ):       out += getLongitudeOption();                                     break;
    CASE( "LANG" ):      out += getLanguageCodeOption();                                  break;
    CASE( "EXCLUDE" ):   out += getExcludeOption();                                       break;
    CASE( "UNITS_M" ):   out += getUnitsOption() == Config::METRIC ? "selected" : "";     break;
    CASE( "UNITS_I" ):   out += getUnitsOption() == Config::IMPERIAL ? "selected" : "";   break;
    // ==========================================
    // Status template parameters
    CASE( "STATUS1" ): {
      if( hasWeatherData() ) {
        String s;
        s.reserve( 250 );
        s += "<b>";
        s += formatTime( weatherData.current.time );
        s += "</b><br/>";
        s += "<img src=\"http://openweathermap.org/img/wn/";
        s += weatherData.current.weather_icon;
        s += "@2x.png\"><br/>";
        s += formatTemperature( weatherData.daily[0].temp_max );
        s += " / ";
        s += formatTemperature( weatherData.daily[0].temp_min );
        s += "<br/>";
        s += weatherData.current.weather_desc;
        out += s;
      }
      break;
    }
    CASE( "STATUS2" ): {
      if( hasWeatherData() ) {
        String s;
        s.reserve( 250 );
        // Day +1
        s += "<b>";
        s += formatTime( weatherData.daily[1].time );
        s += "</b><br/>";
        s += formatTemperature( weatherData.daily[1].temp_max );
        s += " / ";
        s += formatTemperature( weatherData.daily[1].temp_min );
        s += "<br/>";
        s += weatherData.daily[1].weather_desc;
        s += "<br/><br/>";
        // Day +2
        s += "<b>";
        s += formatTime( weatherData.daily[2].time );
        s += "</b><br/>";
        s += formatTemperature( weatherData.daily[2].temp_max );
        s += " / ";
        s += formatTemperature( weatherData.daily[2].temp_min );
        s += "<br/>";
        s += weatherData.daily[2].weather_desc;
        out += s;
      }
      break;
    }
  }
}

/* Private */

String OpenWeatherMapModule::formatTemperature( float value ) {
  String s = String( value, 1 );
  if( getUnitsOption() == Config::IMPERIAL ) {
    s += "&deg;F";
  } else {
    s += "&deg;C";
  }
  return s;
}

String OpenWeatherMapModule::getAppIDOption() {
  return getStringOption( "AppID" );
}

uint16_t OpenWeatherMapModule::getAutoUpdateOption() {
  return getShortOption( "AutoUpd" );
}

String OpenWeatherMapModule::getLatitudeOption() {
  return getStringOption( "Lat" );
}

String OpenWeatherMapModule::getLongitudeOption() {
  return getStringOption( "Lon" );
}

String OpenWeatherMapModule::getExcludeOption() {
  return getStringOption( "Exclude", Config::WEATHER_EXCLUDE );
}

String OpenWeatherMapModule::getLanguageCodeOption() {
  return getStringOption( "LangCode" );
}

Config::WeatherUnits OpenWeatherMapModule::getUnitsOption() {
  return static_cast<Config::WeatherUnits>(getByteOption( "Units", Config::WEATHER_UNITS ));
}

bool OpenWeatherMapModule::hasWeatherData() {
  return weatherData.daily.size() >= 7;
}

void OpenWeatherMapModule::httpEventHandler( mg_connection* nc, int ev, void* ev_data ) {
  switch( ev ) {
    case MG_EV_CONNECT: {
      int status = *(int*) ev_data;
      if( status != 0 ) {
        updateWeatherCleanup();
        const RequestData* data = static_cast<RequestData*>(nc->user_data);
        Log.error( "WEA Failed to connect to %s, %s" CR, data->url.c_str(), strerror(status) );
      }
      break;
    }
    case MG_EV_HTTP_REPLY: {
      http_message* hm = static_cast<http_message*>(ev_data);

      DynamicJsonDocument doc( hm->body.len * 2 );
      DeserializationError rc = deserializeJson( doc, hm->body.p, hm->body.len );
      if( rc != DeserializationError::Ok ) {
        auto msg = Utils::format( Messages::JSON_DECODE_ERROR, rc.c_str() );
        Log.error( "WEA %s" CR, msg.c_str() );
      } else {
        JsonObject json = doc.as<JsonObject>();
        // Root fields
        weatherData.timezone             = json["timezone"].as<char*>();
        // The current weather
        JsonObject current               = json["current"].as<JsonObject>();
        weatherData.current.time         = current["dt"];
        weatherData.current.sunrise      = current["sunrise"];
        weatherData.current.sunset       = current["sunset"];
        weatherData.current.temp         = current["temp"];
        weatherData.current.feels_like   = current["feels_like"];
        weatherData.current.pressure     = current["pressure"];
        weatherData.current.humidity     = current["humidity"];
        weatherData.current.uvi          = current["uvi"];
        weatherData.current.wind_speed   = current["wind_speed"];
        weatherData.current.wind_degrees = current["wind_deg"];
        // The current weather array (part of current weather)
        JsonObject w0                    = current["weather"][0];
        weatherData.current.weather_id   = w0["id"];
        weatherData.current.weather_main = w0["main"].as<char*>();
        weatherData.current.weather_desc = w0["description"].as<char*>();
        weatherData.current.weather_icon = w0["icon"].as<char*>();
        // The daily forecast array
        weatherData.daily.clear();
        JsonArray daily                  = json["daily"];
        for( JsonObject item : daily ) {
          OpenWeather::DailyWeather dw;
          dw.time                        = item["dt"];
          dw.pressure                    = item["pressure"];
          dw.humidity                    = item["humidity"];
          dw.wind_speed                  = item["wind_speed"];
          dw.wind_degrees                = item["wind_deg"];
          //
          JsonObject temp                = item["temp"];
          dw.temp_day                    = temp["day"];
          dw.temp_min                    = temp["min"];
          dw.temp_max                    = temp["max"];
          //
          JsonObject feels               = item["feels_like"];
          dw.feels_like_day              = feels["day"];
          dw.feels_like_night            = feels["night"];
          //
          JsonArray weather              = item["weather"];
          JsonObject w1                  = weather[0];
          dw.weather_id                  = w1["id"];
          dw.weather_main                = w1["main"].as<char*>();
          dw.weather_desc                = w1["description"].as<char*>();
          dw.weather_icon                = w1["icon"].as<char*>();
          //
          weatherData.daily.push_back( dw );
        }
        nc->flags |= MG_F_USER_1;
      }
      nc->flags |= MG_F_SEND_AND_CLOSE;
      break;
    }
    case MG_EV_CLOSE: {
      const RequestData* data = static_cast<RequestData*>(nc->user_data);
      delete data;
      updateWeatherCleanup();
      if( nc->flags & MG_F_USER_1 ) {
        Bus.notify( weatherData );
        Bus.notify( (StatusChangedEvent) { this,"" });
        Log.verbose( "WEA Weather is updated" CR );
      }
      break;
    }
    default:
      break;
  }
}

void OpenWeatherMapModule::updateWeather() {
  if( updateStarted ) {
    Log.notice( "WEA Update already started" CR );
    return;
  }

  String url = WEATHER_API_HOST;
  url += "/data/2.5/onecall?appid=" + getAppIDOption();
  url += "&lat=" + getLatitudeOption();
  url += "&lon=" + getLongitudeOption();
  url += "&units=" + toString( getUnitsOption() );
  // Optional 'exclude' request parameter.
  const String exclude = getExcludeOption();
  if( exclude.length() > 0 ) {
    url += "&exclude=" + exclude;
  }
  // Optional 'lang' request parameter.
  const String lang = getLanguageCodeOption();
  if( lang.length() > 0 ) {
    url += "&lang=" + getLanguageCodeOption();
  }

  updateStarted = true;
  mg_mgr_init( &manager, nullptr );

  mg_connection* nc = mg_connect_http( &manager, [](mg_connection* nc, int ev, void* data) {
    INSTANCE->httpEventHandler( nc, ev, data );
  }, url.c_str(), NULL, NULL );
  if( nc ) {
    RequestData* data = new RequestData();
    data->url = url;
    nc->user_data = data;
  }
}

void OpenWeatherMapModule::updateWeatherCleanup() {
  if( updateStarted ) {
    mg_mgr_free( &manager );
    updateStarted = false;
  }
}

String OpenWeatherMapModule::formatTime( long time ) {
  struct tm tmp;
  localtime_r( &time, &tmp );

  char buf[64];
  strftime( buf, 64, "%a %d", &tmp );
  return String( buf );
}

String OpenWeatherMapModule::toString( const Config::WeatherUnits units ) {
  switch( units ) {
    case Config::METRIC:    return "metric";
    case Config::IMPERIAL:  return "imperial";
    default:                return "unknown";
  }
}

Config::WeatherUnits OpenWeatherMapModule::toUnits( const String& value ) {
  if( value == "M" || value == "metric" )  return Config::METRIC;
  if( value == "I" || value == "imperial" )  return Config::IMPERIAL;
  return Config::UNKNOWN_UNITS;
}