#include <Arduino.h>
#include <ArduinoLog.h>
#include <rom/rtc.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include "Config.h"
#include "Messages.h"
#include "Utils.h"

/* Common static utilities */

String Utils::fileToString( const char* fpath ) {
  File file = SPIFFS.open( fpath );
  if( !file || file.isDirectory() ) {
    Log.verbose( "Failed to open file %s" CR, fpath );
  } else {
    size_t fsize = file.size();
    if( fsize > 0 ) {
      // Read the file into a buffer.
      std::unique_ptr<char[]> buffer(new char[fsize + 1]);
      char* p = buffer.get();
      file.readBytes( p, fsize );
      file.close();
      // Make a string from the buffer.
      *(p + fsize) = '\0';
      return String( p );
    }
  }
  return "";
}

String Utils::getLocalIP() {
  return WiFi.localIP().toString();
}

String Utils::getMacAddress() {
  char buf[30];
  snprintf( buf, sizeof(buf), "%llX", ESP.getEfuseMac() );
  return String( buf );
}

String Utils::getResetReason() {
  RESET_REASON reason = rtc_get_reset_reason(0);
  switch( reason ) {
    case 1  : return "Vbat power on reset";
    case 3  : return "Software reset digital core";
    case 4  : return "Legacy watch dog reset digital core";
    case 5  : return "Deep Sleep reset digital core";
    case 6  : return "Reset by SLC module, reset digital core";
    case 7  : return "Timer Group0 Watch dog reset digital core";
    case 8  : return "Timer Group1 Watch dog reset digital core";
    case 9  : return "RTC Watch dog Reset digital core";
    case 10 : return "Instrusion tested to reset CPU";
    case 11 : return "Time Group reset CPU";
    case 12 : return "Software reset CPU";
    case 13 : return "RTC Watch dog Reset CPU";
    case 14 : return "for APP CPU, reseted by PRO CPU";
    case 15 : return "Reset when the vdd voltage is not stable";
    case 16 : return "RTC Watch dog reset digital core and rtc module";
    default : return "NO_MEAN";
  }
}

String Utils::getSystemVersionName() {
  char version[16];
  snprintf( version, sizeof(version), ("%d.%d.%d"), Config::VERSION_CODE >> 24 & 0xff, Config::VERSION_CODE >> 16 & 0xff, Config::VERSION_CODE >> 8 & 0xff );
  return version;
}

int Utils::getWifiRssiAsQuality( int rssi ) {
    int quality = 0;
    if( rssi <= -100 ) {
        quality = 0;
    }
    else if( rssi >= -50 ) {
        quality = 100;
    } else {
        quality = 2 * (rssi + 100);
    }
    return quality;
}

bool Utils::isBool( const String& str ) {
  if( str == "true" ) return true;
  if( str == "false" ) return true;
  return false;
}

bool Utils::isNumber( const char* str ) {
  byte i = 0;
  bool rc = false;
  if( '-' == str[i] ) i++;
  while( str[i] ) {
    if( '.' == str[i] ) {
      i++;
      break;
    }
    if( !isdigit( str[i] )) return false;
    i++;
    rc = true;
  }
  while( str[i] ) {
    if( !isdigit( str[i] )) return false;
    i++;
    rc = true;
  }
  return rc;
}

bool Utils::isIpAddress( const char* str ) {
  uint32_t addr;
  return parseIpString( str, &addr );
}

ResultData Utils::validateJsonNumber( const JsonObject& json, const char* key, int min, int max ) {
  if( !json.containsKey( key )) {
    return {RC_ERROR, Messages::SETTINGS_MISSED_VALUE + String(key)};
  }
  int v = json[key].as<int>();
  if( v < min || v > max ) {
    return {RC_ERROR, String(key) + Messages::SETTINGS_INVALID_VALUE};
  }
  return RESULT_OK;
}

/**
 * Check if a certain timeout has been reached.
 */
bool Utils::isTimeReached( unsigned long timer ) {
  const long passed = timePassedSince( timer );
  return (passed >= 0);
}

void Utils::setNextTimeInterval( unsigned long& timer, const unsigned long step ) {
  timer += step;
  const long passed = timePassedSince( timer );
  if (passed < 0) { return; }   // Event has not yet happened, which is fine.
  if (static_cast<unsigned long>(passed) > step) {
    // No need to keep running behind, start again.
    timer = millis() + step;
    return;
  }
  // Try to get in sync again.
  timer = millis() + (step - passed);
}

/**
 * Return the time difference as a signed value, taking into account the timers may overflow.
 * Returned timediff is between -24.9 days and +24.9 days.
 * Returned value is positive when "next" is after "prev"
 */
long Utils::timeDifference( unsigned long prev, unsigned long next ) {
  long signed_diff = 0;
  // To cast a value to a signed long, the difference may not exceed half 0xffffffffUL (= 4294967294)
  const unsigned long half_max_unsigned_long = 2147483647u;   // = 2^31 -1
  if (next >= prev) {
    const unsigned long diff = next - prev;
    if (diff <= half_max_unsigned_long) {                     // Normal situation, just return the difference.
      signed_diff = static_cast<long>(diff);                  // Difference is a positive value.
    } else {
      // prev has overflow, return a negative difference value
      signed_diff = static_cast<long>((0xffffffffUL - next) + prev + 1u);
      signed_diff = -1 * signed_diff;
    }
  } else {
    // next < prev
    const unsigned long diff = prev - next;
    if (diff <= half_max_unsigned_long) {                     // Normal situation, return a negative difference value
      signed_diff = static_cast<long>(diff);
      signed_diff = -1 * signed_diff;
    } else {
      // next has overflow, return a positive difference value
      signed_diff = static_cast<long>((0xffffffffUL - prev) + next + 1u);
    }
  }
  return signed_diff;
}

/**
 * Compute the number of milliSeconds passed since timestamp given.
 * Note: value can be negative if the timestamp has not yet been reached.
 */
long Utils::timePassedSince( unsigned long timestamp ) {
  return timeDifference( timestamp, millis() );
}

/**
 * Converts string to bool.
 * Returns:
 * - true when string contains a positive integer;
 * - false when string is zero or negative;
 * - false when the string is not a digit or NULL.
 */
bool Utils::toBool( const char* p ) {
  return p ? atoi( p ) > 0 : false;
}

bool Utils::toBool( const String& str ) {
  return str == "true";
}

/**
 * Converts string to byte. Returns 0 when the string is not a digit or NULL.
 */
uint8_t Utils::toByte( const char* p ) {
  return p ? atoi( p ) : 0;
}

String Utils::toString( const bool value ) {
  return value ? "true" : "false";
}

String Utils::toString( const uint64_t value ) {
  char buffer[32];
  snprintf( buffer, sizeof(buffer), "%llu", value );
  return buffer;
}

// Strings conversion

std::pair<String,String> Utils::split( const String& str, const char separator ) {
  int max = str.length() - 1;
  for( int i = 0; i <= max; i++) {
    if( str.charAt(i) == separator ) {
      return std::pair<String,String>( str.substring( 0, i ), str.substring( i+1 ) );
    }
  }
  return std::pair<String,String>( str, "" );
}

String Utils::format( const char* fmt, const char* arg1 ) {
  char buffer[64];
  snprintf( buffer, sizeof(buffer), fmt, arg1 );
  return buffer;
}

String Utils::format( const char* fmt, const String& arg1 ) {
  char buffer[64];
  snprintf( buffer, sizeof(buffer), fmt, arg1.c_str() );
  return buffer;
}

String Utils::format( const char* fmt, const String& arg1, const String& arg2 ) {
  char buffer[64];
  snprintf( buffer, sizeof(buffer), fmt, arg1.c_str(), arg2.c_str() );
  return buffer;
}

String Utils::formatModuleSettingsTitle( const char* id, const char* name ) {
  char buffer[64];
  snprintf( buffer, sizeof(buffer), Messages::TITLE_MODULE_SETTINGS, name );
  return String( buffer );
}

/**
 * Parse an IP address string like 192.168.0.1 to 32-bit unsigned integer.
 * @param str The pointer to source string.
 * @param dst The address to store the resulting value.
 */
bool Utils::parseIpString( const char* str, uint32_t* dst ) {
  uint8_t *part = (uint8_t*)dst;
  byte i;

  *dst = 0;
  for (i = 0; i < 4; i++) {
    part[i] = strtoul(str, NULL, 10);        // Convert byte
    str = strchr(str, '.');
    if (str == NULL || *str == '\0') {
      break;                                 // No more separators, exit
    }
    str++;                                   // Point to next character after separator
  }
  return (3 == i);
}

uint32_t Utils::parseIpString( const char* str ) {
  uint32_t addr;
  parseIpString( str, &addr );
  return addr;
}

String Utils::toJsonString( const char* key, const bool value ) {
  isDigit( '0');
  char buffer[32];
  snprintf( buffer, sizeof(buffer), R"({"%s":%s})", key, value ? "true" : "false" );
  return String( buffer );
}

String Utils::toJsonString( const char* key, const char* value ) {
  char buffer[Config::JSON_MESSAGE_SIZE];
  if( *value == '{' ) {
    snprintf( buffer, sizeof(buffer), R"({"%s":%s})", key, value );
  } else {
    snprintf( buffer, sizeof(buffer), R"({"%s":"%s"})", key, value );
  }
  return String( buffer );
}

String Utils::toJsonString( const char* key, const int value ) {
  char buffer[32];
  snprintf( buffer, sizeof(buffer), R"({"%s":%d})", key, value );
  return String( buffer );
}

String Utils::toResultsJson( const String& cmd, const String& args, const String& results ) {
  StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
  json["cmd"] = cmd;
  // Payload, i.e. the command arguments, could be optional.
  if( args.length() > 0 ) {
    if( args.indexOf( '{') != -1 ) {
      json["payload"] = serialized( args );
    } else {
      json["payload"] = args;
    }
  }
  if( results.indexOf( '{') != -1 ) {
    json["result"] = serialized( results );
  } else {
    json["result"] = results;
  }
  return json.as<String>();
}