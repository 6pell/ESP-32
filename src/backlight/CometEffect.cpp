#include "backlight/CometEffect.h"
#include "backlight/utils.h"

using namespace Backlight;

CometEffect::CometEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator )
  : DynamicEffect(strip, animator) {
  capabilities.hasSpeed = true;
  capabilities.hasIntensity = true;
}

void CometEffect::perform() {
  animator.StartAnimation( 0, FRAMETIME, [this](const AnimationParam& param) {
    if( param.state == AnimationState_Completed ) {
      uint16_t counter = millis() * (speed >>3) + 1;
      uint16_t index = counter * strip.getPixelsCount() >> 16;
      fadeOut( intensity );

      uint8_t i = map( index, 0, strip.getPixelsCount() - 1, 0, 255 );
      RgbColor c = Utils::colorFromPalette( *palette, i, 255, NOBLEND );
      strip.setPixelColor( index, c );

      animator.RestartAnimation( 0 );
    }
  });
  startTicker();
}