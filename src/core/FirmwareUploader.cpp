#include <Update.h>
#include <StreamString.h>
#include "Messages.h"
#include "core/FirmwareUploader.h"

ResultData FirmwareUploader::begin( const String& action, const int data_size ) {
  // Pre-check: action must be "firmware" or "fs_image"
  int cmd;
  if( action == "firmware" ) {
    cmd = U_FLASH;
  } else if( action == "fs_image" ) {
    cmd = U_SPIFFS;
  } else {
    return {RC_ERROR, Messages::COMMAND_UNKNOWN};
  }
  // Start the firmware upgrade.
  const bool rc = Update.begin( data_size, cmd );
  if( rc ) {
    return {RC_OK, Messages::UPLOAD_STARTED};
  } else {
    return {RC_ERROR, getLastUpdateError()};
  }
}

ResultData FirmwareUploader::uploadDataBlock( const char* data, const uint16_t size ) {
  Update.write( (uint8_t*)data, size );
  if( Update.hasError() ) {
    return {RC_ERROR, getLastUpdateError()};
  } else {
    return {RC_OK, Messages::OK};
  }
}

ResultData FirmwareUploader::end( const bool hasSuccessful ) {
  if( hasSuccessful && Update.end() && Update.isFinished() ) {
    return {RC_OK, Messages::OTA_DONE};
  } else {
    Update.abort();
    return {RC_ERROR, getLastUpdateError()};
  }
}

/* Private */

String FirmwareUploader::getLastUpdateError() {
  StreamString error;
  Update.printError( error );
  error.trim();
  return error;
}
