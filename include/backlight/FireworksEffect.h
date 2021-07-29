#pragma once
#include "DynamicEffect.h"

namespace Backlight {

  class FireworksEffect : public DynamicEffect {
    protected:
      uint16_t aux1;
      uint16_t aux2;

    public:
      FireworksEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator );
      virtual void perform();

    protected:
      void drawFireworks();
  };
}