#pragma once
#include <WString.h>
#include "Messages.h"
#include "Module.h"
#include "core/UploadHandler.h"

class CoreModule : public Module {

/* StateFlags */

typedef union {
  uint8_t data;
  struct {
    uint8_t restart_invoked  : 1;
    uint8_t spare01          : 1;
    uint8_t spare02          : 1;
    uint8_t spare03          : 1;
    uint8_t spare04          : 1;
    uint8_t spare05          : 1;
    uint8_t spare06          : 1;
    uint8_t spare07          : 1;
  };
} StateFlags;

/* CoreModule */

private:
  static constexpr const char* const MODULE_OPTIONS_KEY   = "Options";
  static constexpr const char* const WIRE_OPTION_KEY      = "wire";
  static constexpr const char* const SDA_OPTION_KEY       = "sda";
  static constexpr const char* const SCL_OPTION_KEY       = "scl";

  StateFlags     flags;
  int            eventBusToken;
  int            telemetry_period = 1;             // Telemetry period timer.
  uint8_t        reconfig_delay_counter = 0;       // Seconds, delay before issued reconfigure or restart.
  UploadHandler* uploadHandler = nullptr;

public:
  CoreModule();
  virtual ~CoreModule();
  virtual void         tick_100mS( uint8_t phase );
  // Module identification
  virtual const char*  getId()    { return CORE_MODULE; }
  virtual const char*  getName()  { return Messages::TITLE_CORE_MODULE; }
  // Module Web interface
  virtual const String getModuleWebpage();
  // A generic getData/setData interface
  virtual const String getString( const String& key );
  virtual ResultData   setString( const String& key, const String& value );
  // Data upload interface. Used by WebServerModule to upload firmware.
  virtual ResultData   onDataUploadBegin( const String& action, int data_size );
  virtual ResultData   onDataUploadNextBlock( const char* data, const uint16_t size );
  virtual ResultData   onDataUploadEnd( const bool has_successful );

  void                 performPendingRestart();

protected:
  virtual bool         handleCommand( const String& cmd, const String& args );
  virtual ResultData   handleOption( const String& key, const String& value, Options::Action action );
  virtual void         resolveTemplateKey( const String& key, String& out );

private:
  ResultData           applyExtraOptions( const String& options );
  void                 finalizeDataUpload();
  String               prepareManageModulesHTML();
  void                 publishMqttConnectionInfo2();

  static String        prepareTelemetry();
  static void          publishMqttConnectionInfo1();
  static void          publishNetworkInfo();
  static void          publishTimeInfo();

  static const char*   fromLogLevel( uint8_t level );
  static const char*   fromSleepMode( uint8_t mode );
  static uint8_t       toSleepMode( const String& value );
  static uint8_t       toLogLevel( const String& value );
};
