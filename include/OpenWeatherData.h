#pragma once
#include <vector>
#include <stdint.h>
#include <WString.h>

namespace OpenWeather {

  struct CurrentWeather {
    unsigned long time;             // "dt": 1527015000,
    unsigned long sunrise;          // "sunrise": 1526960448,
    unsigned long sunset;           // "sunrise": 1526960448,
    float         temp;             // "temp": 290.56,
    float         feels_like;       // "feels_like": 290.56,
    uint16_t      pressure;         // "pressure": 1013,
    uint8_t       humidity;         // "humidity": 87,
    float         uvi;              // "uvi": 8.97,
    float         wind_speed;       // "wind_speed": {"speed": 1.5},
    uint16_t      wind_degrees;     // "wind_deg": {deg: 226.505},
    uint16_t      weather_id;       // "id": 521,
    String        weather_main;     // "main": "Rain",
    String        weather_desc;     // "description": "shower rain",
    String        weather_icon;     // "icon": "09d"
  };

  struct DailyWeather {
    unsigned long time;
    float         temp_day;
    float         temp_min;
    float         temp_max;
    float         feels_like_day;
    float         feels_like_night;
    uint16_t      pressure;
    uint8_t       humidity;
    float         wind_speed;
    uint16_t      wind_degrees;
    uint16_t      weather_id;
    String        weather_main;
    String        weather_desc;
    String        weather_icon;
  };

  struct WeatherData {
    String         timezone;
    CurrentWeather current;
    std::vector<DailyWeather> daily;
  };
}