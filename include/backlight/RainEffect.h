#pragma once
#include "FireworksEffect.h"

namespace Backlight {

  class RainEffect : public FireworksEffect {
    private:
      uint16_t counterModeStep;

    public:
      RainEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator );
      virtual void perform();
  };
}