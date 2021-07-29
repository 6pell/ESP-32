#pragma once
#include <lvgl.h>

struct LcdInitCommand {
  const uint8_t  cmd;
  const uint8_t* data;
  const uint8_t  length;      //Nr of bytes in data; bit 7 = delay after set; 0xFF = end of cmds.
};

class DisplayDriver {
public:
  virtual ~DisplayDriver() {};
  virtual void flushArea( lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p ) = 0;
  virtual bool readTouch( lv_indev_drv_t* drv, lv_indev_data_t* data ) {return false;}
  virtual void setBacklight( bool enable ) {};

protected:
  void writeCommandSequence( const LcdInitCommand sequence[] );

  virtual void writeCommand( uint8_t c ) = 0;
  virtual void writeData( uint8_t d ) = 0;
};