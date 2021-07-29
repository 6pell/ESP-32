#include "backlight/RainEffect.h"

using namespace Backlight;

RainEffect::RainEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator )
    : FireworksEffect(strip, animator) {
}

void RainEffect::perform() {
  animator.StartAnimation( 0, FRAMETIME, [this](const AnimationParam& param) {
    if( param.state == AnimationState_Completed ) {
      counterModeStep += 22;
      if( counterModeStep > speedFormulaValue() ) {
        counterModeStep = 0;
        // shift all leds right
        const uint16_t last = strip.getPixelsCount() - 1;
        const RgbColor ctemp = strip.getPixelColor( last );
        for( uint16_t i = last; i > 0; i-- ) {
          strip.setPixelColor( i, strip.getPixelColor( i - 1 ));
        }
        strip.setPixelColor( 0, ctemp );
        aux1++;
        aux2++;
        if( aux1 == 0 ) aux1 = UINT16_MAX;
        if( aux2 == 0 ) aux1 = UINT16_MAX;
        if( aux1 == last + 1 ) aux1 = 0;
        if( aux2 == last + 1 ) aux2 = 0;
      }
      drawFireworks();
      animator.RestartAnimation( 0 );
    }
  });
  startTicker();
}