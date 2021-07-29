#pragma once
#include "core/UploadHandler.h"

class ConfigImporter : public UploadHandler {
private:
  String buffer;
public:
  virtual ResultData begin( const String& action, const int total_size );
  virtual ResultData uploadDataBlock( const char* data, const uint16_t data_size );
  virtual ResultData end( const bool hasSuccessful );
};