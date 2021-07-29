#pragma once
#include <Ticker.h>
#include "Module.h"

class LedClock1Module : public Module {

private:
  const int      REFRESH_RATE = 3;
  const uint8_t  DIGIT_OFF = 11;
  const char     DIGIT_DASH = 10;

  // -- Pin definitions ---------------------------
  const uint8_t  SEGMENT_A_PIN  = 32;
  const uint8_t  SEGMENT_B_PIN  = 13;
  const uint8_t  SEGMENT_C_PIN  = 14;
  const uint8_t  SEGMENT_D_PIN  = 15;
  const uint8_t  SEGMENT_E_PIN  = 16;
  const uint8_t  SEGMENT_F_PIN  = 17;
  const uint8_t  SEGMENT_G_PIN  = 18;
  const uint8_t  SEGMENT_S_PIN  = 19;     // S means symbols column
  const uint8_t  DIGIT_1_PIN    = 23;
  const uint8_t  DIGIT_2_PIN    = 25;
  const uint8_t  DIGIT_3_PIN    = 26;
  const uint8_t  DIGIT_4_PIN    = 27;

  // -- Symbol to pin values conversion table -----
  const uint8_t  SYMBOL_TO_BITSET[12] = {
    0b00000011,                           // 0
    0b10011111,                           // 1
    0b00100101,                           // 2
    0b00001101,                           // 3
    0b10011001,                           // 4
    0b01001001,                           // 5
    0b01000001,                           // 6
    0b00011111,                           // 7
    0b00000001,                           // 8
    0b00001001,                           // 9
    0b11111101,                           // -
    0b11111111                            // OFF
  };

  // -- Pin value to pin conversion table ---------
  const uint8_t BIT_TO_PIN[8] = {
    SEGMENT_A_PIN,
    SEGMENT_B_PIN,
    SEGMENT_C_PIN,
    SEGMENT_D_PIN,
    SEGMENT_E_PIN,
    SEGMENT_F_PIN,
    SEGMENT_G_PIN,
    SEGMENT_S_PIN
  };

  // -- Digit position to pin conversion table -----
  const uint8_t DIGIT_TO_PIN[4] = {
    DIGIT_1_PIN,
    DIGIT_2_PIN,
    DIGIT_3_PIN,
    DIGIT_4_PIN
  };

  uint8_t   displayBuffer[4];
  bool      showDots = false;
  bool      showAlarm = false;
  uint8_t   currentPosition = 0;
  Ticker    ticker;
  int       eventBusToken;

public:
  LedClock1Module();
  virtual void tick_100mS( uint8_t phase );
  virtual ~LedClock1Module();
  // Module identification
  virtual const char*   getId()    { return CLOCK1_MODULE; }
  virtual const char*   getName()  { return Messages::TITLE_CLOCK1_MODULE; }

private:
  void clearDisplay();
  void showNextSymbol();
  void showSymbol( uint8_t symbol, uint8_t position );

  static char digitToSymbol( unsigned char digit );
  static void ticker_callback( LedClock1Module* pThis ) {
    pThis->showNextSymbol();
  }
};