#pragma once
#include "Module.h"

class StatusLedModule : public Module {
public:
  enum LEDMode {     //      -    -    -    -     // LED pattern for each 100 ms in the range
    OFF              = 0b00000000000000000000,
    SLOW_FLASH       = 0b10100000000000000000,    // One double flash in 2 sec
    FAST_FLASH       = 0b10100000001010000000,    // One double flash per second
    SLOWEST_BLINK    = 0b10000000000000000000,    // One blink in 2sec
    SLOW_BLINK       = 0b10000000001000000000,    // One blink per second
    FAST_BLINK       = 0b10000100001000010000,    // One blink in 0.5sec
    SLOWEST_TOGGLE   = 0b11111111110000000000,    // Toggle LED 1sec on / 1sec off
    SLOW_TOGGLE      = 0b11111000001111100000,    // Toggle LED 0.5ec on / 0.5sec off
    ON               = 0b11111111111111111111
  };

private:
  const uint8_t MAX_100MS_STEPS = 20;

  uint8_t ledPin;
  bool    ledInverse;                             // true = invert LED
  LEDMode ledMode;
  bool    ledState;                               // LED On/Off

  uint8_t cycleRepeatCount;
  int8_t  bitPosition;

public:
  StatusLedModule();
  virtual ~StatusLedModule();
  virtual void         tick_100mS( uint8_t phase );
  // Module identification
  virtual const char*  getId()                            { return STATUS_LED_MODULE; }
  virtual const char*  getName()                          { return Messages::TITLE_STATUS_LED_MODULE; }

  void                 continuous( const LEDMode mode )   { oneshot( mode, 255 ); }
  LEDMode              getMode()                          { return ledMode; }
  void                 off()                              { oneshot(OFF,255); }
  void                 oneshot( const LEDMode mode, const uint8_t count = 1 );

protected:
  virtual bool         handleCommand( const String& cmd, const String& args );

private:
  void                 reset();
  void                 setState( bool state );
  String               toJsonString();
  static LEDMode       toLEDMode( const String& value );
  static String        toString( const LEDMode mode );
};