#pragma once
#include "Effect.h"

namespace Backlight {

  class StaticLightEffect : public Effect {
  public:
    StaticLightEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator ) : Effect(strip, animator) {
      capabilities.hasColor = true;
      color = RgbColor( 255 );
    }
    
    virtual void perform() {
      strip.clearTo( color );
      strip.show();
    }
  };
}