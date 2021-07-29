#pragma once
#include <NeoPixelBrightnessBus.h>
#include <NeoPixelAnimator.h>

class NeoPixelWrapper {
private:
  // Fine tune power estimation constants for your setup.

  // Power units per milliamperere for accurate power estimation
  // formula: 195075 divided by mA per fully lit LED, here ~54mA)
  // lowering the value increases the estimated usage and therefore makes the ABL more aggressive
  const uint16_t POWER_UNITS_PER_MA    = 3600;

  // How much mA does the ESP use (Wemos D1 about 80mA, ESP32 about 120mA)
  // You can set it to 0 if the ESP is powered by USB and the LEDs by external
  const uint16_t MCU_POWER_CONSUMPTION = 100;

  typedef Neo800KbpsMethod RgbStripMethod;
  typedef NeoPixelBrightnessBus<NeoGrbFeature, RgbStripMethod> RgbStrip;

  RgbStrip  strip;
  uint8_t   stripPin;
  uint8_t   brightness;
  uint16_t  currentMilliampers = 0;
  uint16_t  maxPowerBudget;

public:
  NeoPixelWrapper( uint16_t pixelsCount, uint8_t pin ) :
    strip( pixelsCount, pin ) {
    stripPin = pin;
  }
  void     begin()                                        { strip.Begin(); }
  bool     canShow()                                      { return strip.CanShow(); }
  void     clearTo( RgbColor color )                      { strip.ClearTo( color ); }
  uint8_t  getBrightness()                                { return brightness; }
  uint8_t  getPin()                                       { return stripPin; }
  const    RgbColor getPixelColor( uint16_t index )       { return strip.GetPixelColor( index ); }
  uint16_t getPixelsCount()                               { return strip.PixelCount(); }
  uint16_t getStripCurrent()                              { return currentMilliampers; }
  void     setBrightness( uint8_t value );
  void     setMaxPowerBudget( uint16_t current )          { maxPowerBudget = current; }
  void     setPixelColor( uint16_t index, RgbColor color) { strip.SetPixelColor( index, color ); }
  void     show();
};