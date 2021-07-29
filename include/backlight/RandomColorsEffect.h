#pragma once
#include "DynamicEffect.h"

namespace Backlight {

  class RandomColorsEffect : public DynamicEffect {
    private:
      const uint16_t COLOR_TRANSITION_TIME =  1*1000;  // 1 sec
      const uint16_t MAX_COLOR_HOLD_TIME   = 30*1000;  // 30 sec

      uint8_t colorIndex;
      RgbColor oldColor;
      RgbColor newColor = HtmlColor( 0 );

    public:
      RandomColorsEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator );
      virtual void perform();

    private:
      void colorTransitionAnimation( const AnimationParam& param );
      void delayAnimation( const AnimationParam& param );
  };
}