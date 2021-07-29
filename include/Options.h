#pragma once
#include <Preferences.h>
#include <stdint.h>
#include <WString.h>
#include "Config.h"

namespace Options {

  /* Possible actions for the handle options API */

  enum Action {READ, VERIFY, SAVE};

  struct StringConstraints {
    uint8_t notEmpty  : 1;    // Value must be not empty
    uint8_t important : 1;    // Value modification requires a module re-initializaton.
  };


  extern Preferences preferences;
  extern uint8_t     sleep_mode;
  extern uint8_t     sleep_time;

  void           clear();
  void           setupPreferences();
  const String   makeKey( const String& moduleId, const String& key );

  const uint8_t  getByte( const String& moduleId, const String& key, uint8_t defValue = 0 );
  const uint32_t getLong( const String& moduleId, const String& key, uint32_t defValue = 0 );
  const uint16_t getShort( const String& moduleId, const String& key, uint16_t defValue = 0 );
  const String   getString( const String& moduleId, const String& key, const String& defValue = "" );

  void           setByte( const String& moduleId, const String& key, uint8_t value );
  void           setLong( const String& moduleId, const String& key, uint32_t value );
  void           setShort( const String& moduleId, const String& key, uint16_t value );
  void           setString( const String& moduleId, const String& key, const String& value );

  // A few options are read very frequently, on each main loop iteration. For better performance
  // they're cached in RAM

  const uint8_t  getSleepMode();
  const uint8_t  getSleepTime();

  void           setSleepMode( const uint8_t value );
  void           setSleepTime( const uint8_t value );
}