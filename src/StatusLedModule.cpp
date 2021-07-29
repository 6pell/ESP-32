#include <ArduinoLog.h>
#include "Config.h"
#include "StatusLedModule.h"
#include "str_switch.h"
#include "Utils.h"

StatusLedModule::StatusLedModule() {
  properties.tick_100mS_required = true;

  ledPin = Config::STATUS_LED_PIN;
  ledInverse = Config::STATUS_LED_INVERSE_PIN;
  ledMode = OFF;
  pinMode( ledPin, OUTPUT );
  reset();
}

StatusLedModule::~StatusLedModule() {
  // Do nothing here
}

void StatusLedModule::tick_100mS( uint8_t phase ) {
  if( ledMode != ON && ledMode != OFF ) {

    bool state = ( ((uint32_t)ledMode) >> bitPosition ) & 1;
    setState( state );

    if( --bitPosition < 0 ) {
      if( cycleRepeatCount == 255 || --cycleRepeatCount > 0 ) {
        bitPosition = MAX_100MS_STEPS - 1;
      } else {
        reset();
      }
    }
  }
}

void StatusLedModule::oneshot( const LEDMode mode, const uint8_t count ) {
  switch( mode ) {
    case ON:
      cycleRepeatCount = 0;
      ledMode = mode;
      setState( HIGH );
      break;

    case OFF:
      reset();
      break;

    default:
      cycleRepeatCount = count;
      ledMode = mode;
      break;
  }
}

/* Protected */

bool StatusLedModule::handleCommand( const String& cmd, const String& args ) {
  const LEDMode mode = toLEDMode( cmd );
  uint8_t count = Utils::toByte( args.c_str() );
  if( count == 0 ) count = 255;
  oneshot( mode, count );
  handleCommandResults( cmd, args, toJsonString() );
  return true;
}

/* Private */

void StatusLedModule::reset() {
  ledMode = OFF;
  cycleRepeatCount = 0;
  bitPosition = MAX_100MS_STEPS - 1;
  setState( LOW );
}

void StatusLedModule::setState( bool state ) {
  ledState = state;
  digitalWrite( ledPin, ledInverse ? !state : state );
}

String StatusLedModule::toJsonString() {
  char buffer[32];
  const String mode = toString( ledMode );
  snprintf( buffer, sizeof(buffer), R"({"mode":"%s"})", mode.c_str() );
  return String( buffer );
}

StatusLedModule::LEDMode StatusLedModule::toLEDMode( const String& value ) {
  SWITCH( value.c_str() ) {
    CASE( "on" ):       return LEDMode::ON;
    CASE( "flash1" ):   return LEDMode::SLOW_FLASH;
    CASE( "flash2" ):   return LEDMode::FAST_FLASH;
    CASE( "blink1" ):   return LEDMode::SLOWEST_BLINK;
    CASE( "blink2" ):   return LEDMode::SLOW_BLINK;
    CASE( "blink3" ):   return LEDMode::FAST_BLINK;
    CASE( "toggle1" ):  return LEDMode::SLOWEST_TOGGLE;
    CASE( "toggle2" ):  return LEDMode::SLOW_TOGGLE;
    DEFAULT_CASE:       return LEDMode::OFF;
  }
}

String StatusLedModule::toString( const LEDMode mode ) {
  switch( mode ) {
    case ON:              return "on";
    case SLOW_FLASH:      return "flash1";
    case FAST_FLASH:      return "flash2";
    case SLOWEST_BLINK:   return "blink1";
    case SLOW_BLINK:      return "blink2";
    case FAST_BLINK:      return "blink3";
    case SLOWEST_TOGGLE:  return "toggle1";
    case SLOW_TOGGLE:     return "toggle2";
    default:              return "off";
  }
}
