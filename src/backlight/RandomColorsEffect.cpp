#include "backlight/RandomColorsEffect.h"
#include "backlight/utils.h"

using namespace Backlight;

RandomColorsEffect::RandomColorsEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator )
  : DynamicEffect(strip, animator) {
  capabilities.hasSpeed = true;
  capabilities.hasPalette = false;
}

void RandomColorsEffect::perform() {
  animator.StartAnimation( 0, COLOR_TRANSITION_TIME, [this](const AnimationParam& param) {
    colorTransitionAnimation( param );
  });
  startTicker();
}

void RandomColorsEffect::colorTransitionAnimation( const AnimationParam& param ) {
  switch( param.state ) {
    case AnimationState_Started: {
      oldColor = newColor;
      colorIndex = Utils::randomWheelIndex( colorIndex );
      newColor = Utils::colorWheelValue( colorIndex );
      break;
    }

    case AnimationState_Progress: {
      const RgbColor color = RgbColor::LinearBlend( oldColor, newColor, param.progress );
      strip.clearTo( color );
      break;
    }

    case AnimationState_Completed: {
      const uint16_t duration = MAX_COLOR_HOLD_TIME / MAX_SPEED * (255 - speed);
      animator.StartAnimation( 1, duration, [this](const AnimationParam& param) {
        delayAnimation( param );
      });
      break;
    }
  }
}

void RandomColorsEffect::delayAnimation( const AnimationParam& param ) {
  if( param.state == AnimationState_Completed ) {
    animator.StartAnimation( 0, COLOR_TRANSITION_TIME, [this](const AnimationParam& param) {
      colorTransitionAnimation( param );
    });
  }
}