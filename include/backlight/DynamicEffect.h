#pragma once
#include <ArduinoLog.h>
#include <Ticker.h>
#include "Effect.h"
#include "RgbPalette.h"

namespace Backlight {

  class DynamicEffect : public Effect {
  private:
    Ticker        ticker;
    String        paletteId;
    uint32_t      lastPaletteChange;

  protected:
    RgbPalette16* palette = nullptr;

  public:
    DynamicEffect( NeoPixelWrapper& strip, NeoPixelAnimator& animator ) : Effect(strip, animator) {
      // By default, all dynamic effects has a palette.
      capabilities.hasPalette = true;
    }

    virtual ~DynamicEffect() {
      ticker.detach();
      if( palette ) delete palette;
    }

    virtual void perform() = 0;

  protected:
    void startTicker() {
      ticker.attach_ms( 1, tickerCallback, this );
    }

    void stopTicker() {
      ticker.detach();
    }

    virtual const String getDefaultPaletteId() {
      return "party";
    }

    virtual void readOptions( const JsonDocument& doc ) {
      Effect::readOptions( doc );
      // palette option
      if( capabilities.hasPalette && doc.containsKey( PALETTE_KEY )) {
        const String id = doc[PALETTE_KEY];
        paletteId = id;
        palette = createPalette( paletteId );
      } else {
        paletteId = getDefaultPaletteId();
        palette = createPalette( paletteId );
      }
    }

  private:
    RgbPalette16* createPalette( const String& id );

    void tickEveryMillisecond( void ) {
      if( animator.IsAnimating() ) {
        animator.UpdateAnimations();
        if( strip.canShow() ) {
          strip.show();
        }
        if( paletteId == "random" ) {
          if( millis() - lastPaletteChange > 1000 + ((uint32_t)(255-intensity)) * 100 ) {
            if( palette ) delete palette;
            palette = createPalette( paletteId );
            lastPaletteChange = millis();
          }
        }
      }
    }

    static void tickerCallback( DynamicEffect* pThis ) {
      pThis->tickEveryMillisecond();
    }
  };
}