#include <ArduinoLog.h>
#include "Options.h"

/* Extern */

Preferences Options::preferences;
uint8_t Options::sleep_mode;
uint8_t Options::sleep_time;

void Options::clear() {
  preferences.clear();
}

void Options::setupPreferences() {
  if( preferences.begin( "project_nvs", false )) {
    // Read the most frequently requested preferences into RAM.
    sleep_mode = preferences.getUChar( "SleepMode", Config::SYSTEM_SLEEP_MODE );
    sleep_time = preferences.getUChar( "SleepTime", Config::SYSTEM_SLEEP_TIME );
  } else {
    Log.error( "Settings: preferences init failed" CR );
  }
}

const String Options::makeKey( const String& moduleId, const String& key ) {
  const String s = moduleId + key;
  if( s.length() <= Config::MAX_PREFERENCES_KEY_LENGTH ) {
    return s;
  } else {
    return moduleId.substring( 0, 6 ) + key.substring( 0, 9 );
  }
}

const uint8_t Options::getByte( const String& moduleId, const String& key, uint8_t defValue ) {
  return preferences.getUChar( makeKey(moduleId, key).c_str(), defValue );
}

const uint32_t Options::getLong( const String& moduleId, const String& key, uint32_t defValue ) {
  return preferences.getULong( makeKey(moduleId, key).c_str(), defValue );
}

const uint16_t Options::getShort( const String& moduleId, const String& key, uint16_t defValue ) {
  return preferences.getUShort( makeKey(moduleId, key).c_str(), defValue );
}

const String Options::getString( const String& moduleId, const String& key, const String& value ) {
  return preferences.getString( makeKey(moduleId, key).c_str(), value );
}

void Options::setByte( const String& moduleId, const String& key, uint8_t value ) {
  preferences.putUChar( makeKey(moduleId, key).c_str(), value );
}

void Options::setLong( const String& moduleId, const String& key, uint32_t value ) {
  preferences.putULong( makeKey(moduleId, key).c_str(), value );
}

void Options::setShort( const String& moduleId, const String& key, uint16_t value ) {
  preferences.putUShort( makeKey(moduleId, key).c_str(), value );
}

void Options::setString( const String& moduleId, const String& key, const String& value ) {
  preferences.putString( makeKey(moduleId, key).c_str(), value );
}

const uint8_t Options::getSleepMode() {
  return sleep_mode;
}

const uint8_t Options::getSleepTime() {
  return sleep_time;
}

void Options::setSleepMode( const uint8_t value ) {
  sleep_mode = value;
  preferences.putUChar( "SleepMode", value );
}

void Options::setSleepTime( const uint8_t value ) {
  sleep_time = value;
  preferences.putUChar( "SleepTime", value );
}
