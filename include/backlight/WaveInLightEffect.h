#pragma once
#include "DynamicEffect.h"

namespace Backlight {

  class WaveInLightEffect : public DynamicEffect {
    private:
      uint16_t lastPixel;
    public:
      WaveInLightEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator );
      virtual void perform();
  };
}