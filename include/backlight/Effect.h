#pragma once
#include <ArduinoJson.h>
#include "types.h"
#include "NeoPixelWrapper.h"

namespace Backlight {

  static constexpr const char* const EFFECT_ID_KEY  = "id";
  static constexpr const char* const BRIGHTNESS_KEY = "bright";
  static constexpr const char* const COLOR_KEY      = "color";
  static constexpr const char* const INTENSITY_KEY  = "int";
  static constexpr const char* const PALETTE_KEY    = "pal";
  static constexpr const char* const SPEED_KEY      = "speed";

  static const uint8_t  MIN_SPEED                   = 0;
  static const uint8_t  MAX_SPEED                   = 255;
  static const uint8_t  DEFAULT_BRIGHTNESS          = 60;
  static const uint8_t  DEFAULT_INTENSITY           = 127;
  static const uint8_t  DEFAULT_SPEED               = 50;
  static const uint8_t  WLED_FPS                    = 42;
  static const uint16_t FRAMETIME                   = 1000 / WLED_FPS;

  /**
   * The basic class of effects hierarchy.
   */
  class Effect {
  protected:
    NeoPixelWrapper&  strip;
    NeoPixelAnimator& animator;

    Capabilities      capabilities;
    RgbColor          color = RgbColor( 255 );
    uint8_t           intensity = DEFAULT_INTENSITY;
    uint8_t           speed = DEFAULT_SPEED;

  public:
    Effect( NeoPixelWrapper& st, NeoPixelAnimator& an ) : strip(st), animator(an) {}
    virtual ~Effect() {}
    virtual void perform() = 0;

    const Capabilities getCapabilities()                 { return capabilities; }
    const RgbColor     getColor()                        { return color; }
    const uint8_t      getIntensity()                    { return intensity; }
    const String       getOptions();
    const uint8_t      getSpeed()                        { return speed; }
    void               setColor( const RgbColor& clr )   { color = clr; }
    void               setOptions( const String& json );
    void               setSpeed( uint8_t sp )            { speed = sp; }

  protected:
    void               blur( uint8_t blur_amount );
    void               fadeOut( uint8_t rate );
    virtual void       readOptions( const JsonDocument& doc );
    uint16_t           speedFormulaValue();
  };
}