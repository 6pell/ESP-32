#pragma once
#include <lvgl.h>
#include <Ticker.h>
#include "Module.h"
#include "lvgl/DisplayDriver.h"

/**
 * The ST7796 480x320 TFT display module.
 */
class ST7796Module : public Module {
private:
  lv_disp_buf_t         bufferInfo;
  lv_color_t            displayBuffer1[LV_HOR_RES_MAX * LV_VER_RES_MAX / 10];
  Ticker                ticker_1ms;
  Ticker                ticker_20ms;
  static DisplayDriver* driver;

public:
  ST7796Module();
  virtual ~ST7796Module();
//  virtual void          tick_100mS( uint8_t phase );
  // Module identification
  virtual const char*   getId()    { return ST7796_MODULE; }
  virtual const char*   getName()  { return Messages::TITLE_ST7796_MODULE; }

protected:
  virtual bool          handleCommand( const String& cmd, const String& args );

private:
  void                  demo_create( void );

  static void flushDisplay( lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p ) {
    driver->flushArea( drv, area, color_p );
  }

  static bool readDisplayTouch( lv_indev_drv_t* drv, lv_indev_data_t* data ) {
    return driver->readTouch( drv, data );
  }

  static void ticker_1ms_callback( ST7796Module* pThis ) {
    lv_tick_inc( 1 );
  }

  static void ticker_20ms_callback( ST7796Module* pThis ) {
    lv_task_handler();
  }
};