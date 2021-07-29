#pragma once
#include <lvgl.h>
#include "Config.h"
#include "lvgl/DisplayDriver.h"

using namespace Config;

static constexpr const LcdInitCommand INIT_COMMANDS[] = {
  {0x01, (uint8_t[]) {}, 0x80},                                               // Software reset
  {0x11, (uint8_t[]) {}, 0x80},                                               // Sleep exit
  {0xF0, (uint8_t[]) {0xC3}, 1},                                              // Command Set control, enable extension command 2 partI.
  {0xF0, (uint8_t[]) {0x96}, 1},                                              // Command Set control, enable extension command 2 partII.
  {0x36, (uint8_t[]) {0xE8}, 1},                                              // Memory Data Access Control MX, MY, RGB mode. Top-Left to right-Buttom, RGB.
  {0x3A, (uint8_t[]) {0x55}, 1},                                              // Interface Pixel Format. Control interface color format set to 16.
  {0xB4, (uint8_t[]) {0x01}, 1},                                              // Column inversion. 1-dot inversion.
  {0XB6, (uint8_t[]) {0x80, 0x02, 0x3B}, 3},                                  // Display Function Control
                                                                              // Bypass
                                                                              // Source Output Scan from S1 to S960, Gate Output scan from G1 to G480, scan cycle=2
                                                                              // LCD Drive Line=8*(59+1)
  {0xE8, (uint8_t[]) {0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33}, 8},    // Display Output Ctrl Adjust
                                                                              // Timing for "Gate start"=25 (Tclk)
                                                                              // Timing for "Gate End"=37 (Tclk), Gate driver EQ function ON
  {0xC1, (uint8_t[]) {0x06}, 1},                                              // Power control2. VAP(GVDD)=3.85+( vcom+vcom offset), VAN(GVCL)=-3.85+( vcom+vcom offset).
  {0xC2, (uint8_t[]) {0xA7}, 1},                                              // Power control3. Source driving current level=low, Gamma driving current level=High.
  {0xC5, (uint8_t[]) {0x18}, 0x80+1},                                         // VCOM Control. VCOM=0.9.
  {0xE0, (uint8_t[]) {0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15, 0x2F, 0x54, 0x42, 0x3C, 0x17, 0x14, 0x18, 0x1B}, 14},       // ST7796 Gamma Sequence. Gamma"+"
  {0xE1, (uint8_t[]) {0xE0, 0x09, 0x0B, 0x06, 0x04, 0x03, 0x2B, 0x43, 0x42, 0x3B, 0x16, 0x14, 0x17, 0x1B}, 0x80+14 }, // ST7796 Gamma Sequence. Gamma"-"
  {0x53, (uint8_t[]) {0x00}, 1},
  {0xF0, (uint8_t[]) {0x3C}, 1},                                              // Command Set control. Disable extension command 2 partI.
  {0xF0, (uint8_t[]) {0x69}, 1},                                              // Command Set control. Disable extension command 2 partII.
  {0x00, (uint8_t[]) {}, 0xFF}                                                // End of sequence.
};

static const int    TOUCH_SPI_CLOCK  = 2500000;        // 2.5 MHz
static const int8_t TOUCH_CMD_X_READ = 0b10010000;
static const int8_t TOUCH_CMD_Y_READ = 0b11010000;


// Mask for the 8 data bits to set pin directions
static constexpr uint32_t DIR_MASK = (
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
static constexpr uint32_t CLR_MASK = (DIR_MASK | (1 << ST7796_WR_PIN));


class ST7796Driver : public DisplayDriver {
private:
  uint32_t xset_mask[256];                  // Lookup table for ESP32 parallel bus interface uses 1kbyte RAM
  int16_t  last_x;
  int16_t  last_y;
  int16_t  avg_buf_x[ST7796_TOUCH_AVG];
  int16_t  avg_buf_y[ST7796_TOUCH_AVG];
  uint8_t  avg_last;

public:
  ST7796Driver();
  virtual ~ST7796Driver();
  virtual void flushArea( lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p );
  virtual bool readTouch( lv_indev_drv_t* drv, lv_indev_data_t* data );
  virtual void setBacklight( bool enable );

protected:
  virtual void writeCommand( uint8_t c );    // Send a command, function resets DC/RS high ready for data
  virtual void writeData( uint8_t d );       // Send data with DC/RS set high

private:
  // Display low level
  void   setWindow( int16_t x0, int16_t y0, int16_t x1, int16_t y1 );
  void   write8( uint8_t d );
  void   write16( uint16_t d );
  void   write32c( uint16_t c, uint16_t d );

  inline static void  cs_high();
  inline static void  cs_low();
  inline static void  dc_c();
  inline static void  dc_d();
  inline static void  wr_l();
  inline static void  wr_h();

  // Touch low level
  static int16_t read_spi( uint8_t reg );
  void           xpt2046_avg( int16_t* x, int16_t* y );
  static void    xpt2046_corr( int16_t* x, int16_t* y );
};