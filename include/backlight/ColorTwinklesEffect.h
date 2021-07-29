#pragma once
#include "DynamicEffect.h"

namespace Backlight {

  class ColorTwinklesEffect : public DynamicEffect {
    private:
      std::vector<bool> data;
      fract8 fadeUpAmount;
      fract8 fadeDownAmount;

    public:
      ColorTwinklesEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator );
      virtual void perform();

    private:
      void draw();
  };
}