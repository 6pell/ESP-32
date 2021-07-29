#include "Config.h"
#include "backlight/NeoPixelWrapper.h"

// Power limit calculation.
// Each LED can draw up 195075 "power units" (approx. 53mA).
// One PU is the power it takes to have 1 channel 1 step brighter per brightness step
// so A=2,R=255,G=0,B=0 would use 510 PU per LED (1mA is about 3700 PU)
void NeoPixelWrapper::show() {
  if( maxPowerBudget > 0 ) {        // zero turns off calculation
    uint32_t powerBudget = (maxPowerBudget - MCU_POWER_CONSUMPTION) * POWER_UNITS_PER_MA;
    uint16_t pixelsCount = strip.PixelCount();

    // each LED uses about 1mA in standby, exclude that from power budget
    if( powerBudget > POWER_UNITS_PER_MA * pixelsCount ) {
      powerBudget -= POWER_UNITS_PER_MA * pixelsCount;
    } else {
      powerBudget = 0;
    }

    // sum up the usage of each LED
    uint32_t powerSum = 0;
    for( uint16_t i = 0; i < pixelsCount; i++ ) {
      RgbColor c = strip.GetPixelColor( i );
      if( Config::BACKLIGHT_WS8212B_ECO ) {
        powerSum += (c.R + c.G + c.B) / 2;
      } else {
        powerSum += (c.R + c.G + c.B);
      }
    }

    uint32_t powerSum0 = powerSum;
    powerSum *= brightness;

    // scale brightness down to stay in current limit
    if( powerSum > powerBudget ) {
      float scale = (float)powerBudget / (float)powerSum;
      uint16_t scaleI = scale * 255;
      uint8_t scaleB = (scaleI > 255) ? 255 : scaleI;

      // scale one byte by a second one, which is treated as
      // the numerator of a fraction whose denominator is 256
      // In other words, it computes i * (scale / 256)
      uint8_t newBrightness = ((uint16_t)brightness * (uint16_t)scaleB ) >> 8;

      strip.SetBrightness( newBrightness );
      currentMilliampers = (powerSum0 * newBrightness) / POWER_UNITS_PER_MA;
    } else {
      currentMilliampers = powerSum / POWER_UNITS_PER_MA;
      strip.SetBrightness( brightness );
    }
    // add power of ESP and LED standby power back to estimate
    currentMilliampers += MCU_POWER_CONSUMPTION;
    currentMilliampers += pixelsCount;
  } else {
    currentMilliampers = 0;
  }
  strip.Show();
}

void NeoPixelWrapper::setBrightness( uint8_t value ) {
  if( brightness == value ) return;
  brightness = value;
  strip.SetBrightness( value );
  show();
}