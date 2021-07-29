#pragma once
#include "core/UploadHandler.h"

class FirmwareUploader : public UploadHandler {
public:
  virtual ResultData begin( const String& action, const int data_size );
  virtual ResultData uploadDataBlock( const char* data, const uint16_t size );
  virtual ResultData end( const bool hasSuccessful );
private:
  static String      getLastUpdateError();
};