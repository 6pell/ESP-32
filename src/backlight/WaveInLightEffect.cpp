#include "backlight/WaveInLightEffect.h"

using namespace Backlight;

WaveInLightEffect::WaveInLightEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator )
    : DynamicEffect(strip, animator) {
  capabilities.hasColor = true;
  capabilities.hasSpeed = true;
}

void WaveInLightEffect::perform() {
  lastPixel = 0;
  uint16_t delay = speedFormulaValue();
  animator.StartAnimation( 0, delay, [this]( const AnimationParam& param ) {
    if( param.state == AnimationState_Completed ) {
      if( lastPixel < strip.getPixelsCount() ) {
        strip.setPixelColor( lastPixel++, color );
        animator.RestartAnimation( 0 );
      } else {
        stopTicker();
      }
    }
  });
  // Start animation.
  startTicker();
}
