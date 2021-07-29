#pragma once
#include "DynamicEffect.h"

namespace Backlight {

  // Firing comets from one end.
  class CometEffect : public DynamicEffect {
    public:
      CometEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator );
      virtual void perform();
  };
}