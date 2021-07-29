#include <ArduinoJson.h>
#include "config.h"
#include "Messages.h"
#include "ModulesManager.h"
#include "Utils.h"
#include "core/ConfigImporter.h"

ResultData ConfigImporter::begin( const String& action, const int total_size ) {
  buffer.reserve( total_size );
  return {RC_OK, Messages::UPLOAD_STARTED};
}

ResultData ConfigImporter::uploadDataBlock( const char* data, const uint16_t data_size ) {
  String s;
  uint16_t size = data_size;
  if( size > 0 ) {
    s.reserve( size );
    do {
      s += *data++;
    } while( --size );
  }

  buffer += s;
  return {RC_OK, Messages::OK};
}

ResultData ConfigImporter::end( const bool hasSuccessful ) {
  if( hasSuccessful ) {
    DynamicJsonDocument doc( Config::JSON_EXPORT_CONFIG_SIZE );
    DeserializationError rc = deserializeJson( doc, buffer );
    // If the JSON config cannot be decoded.
    if( rc != DeserializationError::Ok ) {
      String msg = Utils::format( Messages::JSON_DECODE_ERROR, rc.c_str() );
      Log.error( "IMPORT %s" CR, msg );
      return {RC_OK, msg};
    }
    // Separate the config JSON into sub-objects. Provide an each sub-object to
    // the respective module.
    JsonObject json = doc.as<JsonObject>();
    for( JsonPair p : json ) {
      const String moduleId = p.key().c_str();
      const String data = p.value().as<String>();

      Module* module = Modules.get( moduleId );
      if( module ) {
        const ResultData rc = module->setString( Config::KEY_IMPORT_CONFIGURATION, data );
        if( rc.code != RC_OK && rc.code != RC_OK_REINIT ) {
          Log.error( "IMPORT %s %s" CR, module->getId(), rc.details.c_str() );
          return rc;
        }
      }
    }
  }
  return {RC_OK, Messages::UPLOAD_COMPLETE};
}

