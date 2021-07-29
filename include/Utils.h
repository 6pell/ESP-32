#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <stdint.h>
#include "core/ResultData.h"

namespace Utils {

  const char hex_chars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

  // File read/write related
  String     fileToString( const char* fpath );

  // System related
  String     getLocalIP();
  String     getMacAddress();
  String     getResetReason();
  String     getSystemVersionName();
  int        getWifiRssiAsQuality( int rssi );

  // String conversions
  String     format( const char* fmt, const char* arg1 );
  String     format( const char* fmt, const String& arg1 );
  String     format( const char* fmt, const String& arg1, const String& arg2 );
  String     formatModuleSettingsTitle( const char* id, const char* name );
  bool       parseIpString( const char* str, uint32_t* dst );
  uint32_t   parseIpString( const char* str );
  std::pair<String,String> split( const String& str, const char separator = ' ' );
  String     toJsonString( const char* key, const bool value );
  String     toJsonString( const char* key, const char* value );
  String     toJsonString( const char* key, const int value );
  String     toResultsJson( const String& cmd, const String& args, const String& result );

  // Time management
  bool       isTimeReached( unsigned long timer );
  void       setNextTimeInterval( unsigned long& timer, const unsigned long step );

  // Type conversion
  bool       toBool( const char* p );
  bool       toBool( const String& str );
  uint8_t    toByte( const char* p );
  String     toString( const bool value );
  String     toString( const uint64_t value );

  // Validation and data checking
  bool       isBool( const String& str );
  bool       isNumber( const char* str );
  bool       isIpAddress( const char* str );
  ResultData validateJsonNumber( const JsonObject& json, const char* key, int min, int max );

  // Internal
  long       timeDifference( unsigned long prev, unsigned long next );
  long       timePassedSince( unsigned long timestamp );
};