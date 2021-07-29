#pragma once
#include "DynamicEffect.h"

namespace Backlight {

  class Noise1Effect : public DynamicEffect {
    private:
      const uint16_t scale = 320;     // The "zoom factor" for the noise
      uint32_t step = 0;

    public:
      Noise1Effect( NeoPixelWrapper& strip, NeoPixelAnimator& animator );
      virtual void perform();

    protected:
      virtual const String getDefaultPaletteId() {
        return "drywet";
      }
  };
}