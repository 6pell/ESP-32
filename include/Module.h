#pragma once
#include <functional>
#include <map>
#include <stdint.h>
#include <WString.h>
#include <ArduinoJson.h>
#include "ModuleId.h"
#include "Options.h"
#include "core/ResultData.h"

// ====================================
/* Module */

class Module {
public:
  union Properties {
    uint8_t data;
    struct {
      uint8_t loop_required       : 1;
      uint8_t tick_100mS_required : 1;
      uint8_t has_module_webpage  : 1;    // True if module has a module webpage.
      uint8_t has_status_webpage  : 1;    // True if module has a status webpage.
      uint8_t spare04             : 1;
      uint8_t spare05             : 1;
      uint8_t spare06             : 1;
      uint8_t spare07             : 1;
    };
  };

protected:
  /* Aliases of options constraints */
  static const bool NOT_EMPTY = true;     // Option's value must be non-empty.
  static const bool OPTIONAL  = false;    // Option's value can be empty, i.e. optional.
  static const bool IMPORTANT = true;     // A modification of option requires the module re-initialization.

// ========================================================
/* The module class itself */

protected:
  Properties properties;

public:
  Module() { properties.data = 0; }
  virtual ~Module() {}

  // Module management
  virtual void          loop() {}
  virtual void          tick_100mS( uint8_t phase ) {}

  // Module identification
  virtual const char*   getId() = 0;
  virtual const char*   getName() = 0;

  // Module Web interface
  virtual const String  getModuleWebpage();
  virtual const String  getStatusWebpage();

  // Data upload interface. Used by WebServerModule to upload firmware.
  virtual ResultData    onDataUploadBegin( const String& action, int data_size )        { return UNKNOWN_COMMAND; }
  virtual ResultData    onDataUploadNextBlock( const char* data, const uint16_t size )  { return UNKNOWN_COMMAND; }
  virtual ResultData    onDataUploadEnd( const bool has_successful )                    { return UNKNOWN_COMMAND; }

  // A generic getData/setData interface
  virtual const uint8_t getByte( const String& key );
  virtual const String  getString( const String& key ) {return "";}
  virtual void          setByte( const String& key, const uint8_t value );
  virtual ResultData    setString( const String& key, const String& value );

  bool                  dispatchCommand( const String& command );
  ResultData            dispatchSettings( const std::map<String,String>& map );
  const Properties      getProperties()  { return properties; }
  virtual void          reinitModule() {;}

protected:
  virtual bool          handleCommand( const String& cmd, const String& args )  { return false; }
  virtual void          handleCommandResults( const String& cmd, const String& args, const String& result );
  virtual ResultData    handleOption( const String& key, const String& value, Options::Action action ) { return UNKNOWN_OPTION; }
  virtual void          resolveTemplateKey( const String& key, String& out ) {;}

  // Non-virtual protected methods

  const String          getMacroOption( const String& optionKey, const String& defValue = "" ) {
    return getMacroOptionOf( getId(), optionKey, defValue );
  }

  const String          getMacroOptionOf( const String& moduleId, const String& optionKey, const String& defValue = "" );
  ResultData            handleConfigImport( const String& data );
  String                makeWebpage( const char* fpath );
  ResultData            handleByteOption( const String& key, const String& value, Options::Action action, bool important );
  ResultData            handleIpAddressOption( const String& key, const String& value, Options::Action action, Options::StringConstraints cs );
  ResultData            handleLongOption( const String& key, const String& value, Options::Action action, bool important );
  ResultData            handleShortOption( const String& key, const String& value, Options::Action action, bool important );
  ResultData            handleStringOption( const String& key, const String& value, Options::Action action, Options::StringConstraints cs );

  // Preferences API

  const uint8_t getByteOption( const String& key, const uint8_t defValue = 0 ) {
    return Options::getByte( getId(), key, defValue );
  }

  const uint32_t getLongOption( const String& key, const uint32_t defValue = 0 ) {
    return Options::getLong( getId(), key, defValue );
  }

  const uint16_t getShortOption( const String& key, const uint16_t defValue = 0 ) {
    return Options::getShort( getId(), key, defValue );
  }

  const String getStringOption( const String& key, const String& defValue = "" ) {
    return Options::getString( getId(), key, defValue );
  }

  void setByteOption( const String& key, const uint8_t value ) {
    Options::setByte( getId(), key, value );
  }

  void setLongOption( const String& key, const uint32_t value ) {
    Options::setLong( getId(), key, value );
  }

  void setShortOption( const String& key, const uint16_t value ) {
    Options::setShort( getId(), key, value );
  }

  void setStringOption( const String& key, const String& value ) {
    Options::setString( getId(), key, value );
  }
};
