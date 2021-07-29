#pragma once
#include "DynamicEffect.h"

namespace Backlight {

  // Water ripple. Propagation velocity from speed. Drop rate from intensity.
  class RippleEffect : public DynamicEffect {
    private:
      struct RippleData {
        uint8_t  state = 0;
        uint16_t waveOrigin;
        uint8_t  colorIndex;
      };

      uint16_t maxRipples;
      RgbColor fillColor;
      std::vector<RippleData*> store;

    public:
      RippleEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator );
      virtual ~RippleEffect();
      virtual void perform();

    private:
      void drawWave();
  };
}