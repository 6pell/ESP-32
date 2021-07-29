#pragma once
#include <Ticker.h>
#include "Module.h"
#include "Messages.h"
#include "backlight/Effect.h"

class BackLightModule : public Module {

/* BackLightModule */

private:
  static constexpr const char* const MODULE_CONFIG_KEY    = "Config";
  static constexpr const char* const FIX_WHITE_OPTION_KEY = "FixWhite";
  static constexpr const char* const MAX_POWER_OPTION_KEY = "MaxPower";
  static constexpr const char* const PIN_OPTION_KEY       = "Pin";
  static constexpr const char* const PIXELS_OPTION_KEY    = "Pixels";

  NeoPixelWrapper* strip;
  NeoPixelAnimator animations = NeoPixelAnimator( 2, NEO_MILLISECONDS );
  Backlight::Effect* effect = nullptr;
  RgbColor fixWhiteColor;
  String   lastEffectId;

public:
  BackLightModule();
  virtual ~BackLightModule();
  // Module identification
  virtual const char* getId()    { return BACKLIGHT_MODULE; }
  virtual const char* getName()  { return Messages::TITLE_BACKLIGHT_MODULE; }
  // Module Web interface
  virtual const String getModuleWebpage();
  // A generic getData/setData interface
  virtual const String getString( const String& key );
  virtual ResultData   setString( const String& key, const String& value );

protected:
  virtual bool       handleCommand( const String& cmd, const String& args );
  virtual ResultData handleOption( const String& key, const String& value, Options::Action action );
  virtual void       resolveTemplateKey( const String& key, String& out );

private:
  void   performEffect( const String& jsonOptions );
  void   reset( bool resetStrip );

  String buildEffectsHtmlOptions();
  String buildPalettesHtmlOptions();
  String getJsonConfig();
  void   setJsonConfig( const String& cfg );

  static void appendOption( String& out, const String& id, const String& title );
  static RgbColor getFixWhiteOption( const JsonObject& json );
  static uint16_t getMaxPowerBudget( const JsonObject& json );
  static String makeDefaultEffectOptions( const String& effectId );
};