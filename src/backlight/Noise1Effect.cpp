#include "backlight/Noise1Effect.h"
#include "backlight/utils.h"

using namespace Backlight;

Noise1Effect::Noise1Effect( NeoPixelWrapper& strip, NeoPixelAnimator& animator )
  : DynamicEffect(strip, animator) {
  capabilities.hasSpeed = true;
  //capabilities.hasIntensity = true;
}

void Noise1Effect::perform() {
  animator.StartAnimation( 0, FRAMETIME, [this](const AnimationParam& param) {
    if( param.state == AnimationState_Completed ) {
      RgbColor fastled_col;
      step += (1 + speed / 16);

      for( uint16_t i = 0; i < strip.getPixelsCount(); i++ ) {
        uint16_t shift_x = Utils::beatsin8( 11 );                 // the x position of the noise field swings @ 17 bpm
        uint16_t shift_y = step / 42;                             // the y position becomes slowly incremented

        uint16_t real_x = (i + shift_x) * scale;                  // the x position of the noise field swings @ 17 bpm
        uint16_t real_y = (i + shift_y) * scale;                  // the y position becomes slowly incremented
        uint32_t real_z = step;                                   // the z position becomes quickly incremented

        uint8_t noise = Utils::inoise16( real_x, real_y, real_z ) >> 8;  // get the noise data and scale it down
        uint8_t index = Utils::sin8( noise * 3 );                        // map LED color based on noise data

        // With that value, look up the 8 bit colour palette value and assign it to the current LED.
        fastled_col = Utils::colorFromPalette( *palette, index, 255, LINEARBLEND );
        strip.setPixelColor( i, fastled_col );
      }

      animator.RestartAnimation( 0 );
    }
  });
  startTicker();
}