/**
 * The ST7796 TFT display driver for ESP32.
 * It's assumed that the display is working in parallel 8-bit mode.
 *
 * This code is based on the TFT_eSPI Arduino TFT graphics library
 * https://github.com/Bodmer/TFT_eSPI
 */

#pragma once

#include <lvgl.h>
#include "Config.h"

namespace ST7796 {
  using namespace Config;

  const int8_t CMD_X_READ = 0b10010000;
  const int8_t CMD_Y_READ = 0b11010000;
  const int    SPI_CLOCK  = 1000000;        // 1 MHz

  extern uint32_t xset_mask[256];           // Lookup table for ESP32 parallel bus interface uses 1kbyte RAM
  extern int16_t last_x;
  extern int16_t last_y;
  extern int16_t avg_buf_x[Config::ST7796_TOUCH_AVG];
  extern int16_t avg_buf_y[Config::ST7796_TOUCH_AVG];
  extern uint8_t avg_last;


  // LVGL required functions
  void init( void );
  void flush( lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p );
  bool readTouch( lv_indev_drv_t* drv, lv_indev_data_t* data );
  void setBrigthness( uint8_t v );

  // Low level

  // Mask for the 8 data bits to set pin directions
  constexpr uint32_t dir_mask = (
    (1 << ST7796_D0_PIN) |
    (1 << ST7796_D1_PIN) |
    (1 << ST7796_D2_PIN) |
    (1 << ST7796_D3_PIN) |
    (1 << ST7796_D4_PIN) |
    (1 << ST7796_D5_PIN) |
    (1 << ST7796_D6_PIN) |
    (1 << ST7796_D7_PIN)
  );

  // Data bits and the write line are cleared to 0 in one step
  constexpr uint32_t clr_mask = (dir_mask | (1 << ST7796_WR_PIN));

  inline void cs_high();
  inline void cs_low();
  inline void dc_c();
  inline void dc_d();
  inline void wr_l();
  inline void wr_h();

  // void     bus_direction( uint32_t mask, uint8_t mode );
  // uint8_t  read_8( uint8_t c );
  // uint16_t read_16( uint8_t c );

  int16_t  read_spi( uint8_t reg );

  void     write_8( uint8_t d );
  void     write_16( uint16_t d );
  void     write_32c( uint16_t c, uint16_t d );
  void     xpt2046_avg( int16_t* x, int16_t* y );
  void     xpt2046_corr( int16_t* x, int16_t* y );

  void     setWindow( int16_t x0, int16_t y0, int16_t x1, int16_t y1 );
  void     writeCommand( uint8_t c );    // Send a command, function resets DC/RS high ready for data
  void     writeData( uint8_t d );       // Send data with DC/RS set high
};
