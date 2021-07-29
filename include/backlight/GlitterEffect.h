#pragma once
#include "DynamicEffect.h"

namespace Backlight {

  class GlitterEffect : public DynamicEffect {
    public:
      GlitterEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator );
      virtual void perform();

    private:
      void drawPalette();
  };
}