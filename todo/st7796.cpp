#include <SPI.h>
#include "lvgl/st7796.h"

uint32_t ST7796::xset_mask[256];
int16_t last_x = 0;
int16_t last_y = 0;
int16_t avg_buf_x[Config::ST7796_TOUCH_AVG];
int16_t avg_buf_y[Config::ST7796_TOUCH_AVG];
uint8_t avg_last = 0;

/* LVGL required functions */

void ST7796::init( void ) {
  // Create a bit set lookup table for data bus - wastes 1kbyte of RAM but speeds things up dramatically
  // can then use e.g. GPIO.out_w1ts = set_mask(0xFF); to set data bus to 0xFF
  for( int32_t c = 0; c < 256; c++ ) {
    xset_mask[c] = 0;
    if ( c & 0x01 ) xset_mask[c] |= (1 << ST7796_D0_PIN);
    if ( c & 0x02 ) xset_mask[c] |= (1 << ST7796_D1_PIN);
    if ( c & 0x04 ) xset_mask[c] |= (1 << ST7796_D2_PIN);
    if ( c & 0x08 ) xset_mask[c] |= (1 << ST7796_D3_PIN);
    if ( c & 0x10 ) xset_mask[c] |= (1 << ST7796_D4_PIN);
    if ( c & 0x20 ) xset_mask[c] |= (1 << ST7796_D5_PIN);
    if ( c & 0x40 ) xset_mask[c] |= (1 << ST7796_D6_PIN);
    if ( c & 0x80 ) xset_mask[c] |= (1 << ST7796_D7_PIN);
  }

  // Set to output once again in case D6 (MISO) is used for CS
  if( Config::ST7796_CS_PIN != -1 ) {
    pinMode( Config::ST7796_CS_PIN, OUTPUT );
    digitalWrite( Config::ST7796_CS_PIN, HIGH );  // Chip select high (inactive)
  }

  pinMode( ST7796_WR_PIN, OUTPUT );
  digitalWrite( ST7796_WR_PIN, HIGH );                // Set write strobe high (inactive)

  // Set to output once again in case D6 (MISO) is used for DC
  if( Config::ST7796_DC_PIN != -1 ) {
    pinMode( Config::ST7796_DC_PIN, OUTPUT );
    digitalWrite( Config::ST7796_DC_PIN, HIGH );  // Data/Command high = data mode
  }
  cs_high();

  if( ST7796_RST_PIN != -1 ) {
    pinMode( ST7796_RST_PIN, OUTPUT );
    digitalWrite( ST7796_RST_PIN, HIGH );
  }

  // Make sure read is high before we set the bus to output
  pinMode( ST7796_RD_PIN, OUTPUT );
  digitalWrite( ST7796_RD_PIN, HIGH );

  // Set TFT data bus lines to output
  pinMode( ST7796_D0_PIN, OUTPUT ); digitalWrite( ST7796_D0_PIN, HIGH );
  pinMode( ST7796_D1_PIN, OUTPUT ); digitalWrite( ST7796_D1_PIN, HIGH );
  pinMode( ST7796_D2_PIN, OUTPUT ); digitalWrite( ST7796_D2_PIN, HIGH );
  pinMode( ST7796_D3_PIN, OUTPUT ); digitalWrite( ST7796_D3_PIN, HIGH );
  pinMode( ST7796_D4_PIN, OUTPUT ); digitalWrite( ST7796_D4_PIN, HIGH );
  pinMode( ST7796_D5_PIN, OUTPUT ); digitalWrite( ST7796_D5_PIN, HIGH );
  pinMode( ST7796_D6_PIN, OUTPUT ); digitalWrite( ST7796_D6_PIN, HIGH );
  pinMode( ST7796_D7_PIN, OUTPUT ); digitalWrite( ST7796_D7_PIN, HIGH );

  // Toggle RST low to reset
  cs_low();
  if( ST7796_RST_PIN != -1 ) {
    pinMode( ST7796_RST_PIN, OUTPUT );
    digitalWrite( ST7796_RST_PIN, HIGH );
    delay( 5 );
    digitalWrite( ST7796_RST_PIN, LOW );
    delay( 20 );
    digitalWrite( ST7796_RST_PIN, HIGH );
  }
  cs_high();
  delay( 150 );                         // Wait for reset to complete

  // -----------------
  cs_low();
	delay( 120 );

	writeCommand( 0x01 );                 // Software reset
	delay( 120 );

	writeCommand( 0x11 );                 // Sleep exit
	delay( 120 );

	writeCommand( 0xF0 );                 // Command Set control
	writeData( 0xC3 );                    // Enable extension command 2 partI

	writeCommand( 0xF0 );                 // Command Set control
	writeData( 0x96 );                    // Enable extension command 2 partII

	writeCommand( 0x36 );                 // Memory Data Access Control MX, MY, RGB mode
	writeData( 0xE8 );                    // Top-Left to right-Buttom, RGB

	writeCommand( 0x3A );                 // Interface Pixel Format
	writeData( 0x55 );                    // Control interface color format set to 16

	writeCommand( 0xB4 );                 // Column inversion
	writeData( 0x01 );                    // 1-dot inversion

	writeCommand( 0xB6 );                 // Display Function Control
	writeData( 0x80 );                    // Bypass
	writeData( 0x02 );                    // Source Output Scan from S1 to S960, Gate Output scan from G1 to G480, scan cycle=2
	writeData( 0x3B );                    // LCD Drive Line=8*(59+1)


	writeCommand( 0xE8);                  // Display Output Ctrl Adjust
	writeData( 0x40 );
	writeData( 0x8A );
	writeData( 0x00 );
	writeData( 0x00 );
	writeData( 0x29 );                    // Source eqaulizing period time= 22.5 us
	writeData( 0x19 );                    // Timing for "Gate start"=25 (Tclk)
	writeData( 0xA5 );                    // Timing for "Gate End"=37 (Tclk), Gate driver EQ function ON
	writeData( 0x33 );

	writeCommand( 0xC1 );                 // Power control2
	writeData( 0x06 );                    // VAP(GVDD)=3.85+( vcom+vcom offset), VAN(GVCL)=-3.85+( vcom+vcom offset)

	writeCommand( 0xC2 );                 // Power control 3
	writeData( 0xA7 );                    // Source driving current level=low, Gamma driving current level=High

	writeCommand( 0xC5 );                 // VCOM Control
	writeData( 0x18 );                    // VCOM=0.9

	delay( 120 );

	// ST7796 Gamma Sequence
	writeCommand( 0xE0 );                 // Gamma"+"
	writeData( 0xF0 );
	writeData( 0x09 );
	writeData( 0x0b );
	writeData( 0x06 );
	writeData( 0x04 );
	writeData( 0x15 );
	writeData( 0x2F );
	writeData( 0x54 );
	writeData( 0x42 );
	writeData( 0x3C );
	writeData( 0x17 );
	writeData( 0x14 );
	writeData( 0x18 );
	writeData( 0x1B );

	writeCommand( 0xE1 );                 // Gamma"-"
	writeData( 0xE0 );
	writeData( 0x09 );
	writeData( 0x0B );
	writeData( 0x06 );
	writeData( 0x04 );
	writeData( 0x03 );
	writeData( 0x2B );
	writeData( 0x43 );
	writeData( 0x42 );
	writeData( 0x3B );
	writeData( 0x16 );
	writeData( 0x14 );
	writeData( 0x17 );
	writeData( 0x1B );

  delay( 120 );

	writeCommand( 0x53 );
	writeData( 0x00 );

	writeCommand( 0xF0 );                 // Command Set control
	writeData( 0x3C );                    // Disable extension command 2 partI

	writeCommand( 0xF0 );                 // Command Set control
	writeData( 0x69 );                    // Disable extension command 2 partII

  cs_high();
  delay( 120 );
  cs_low();

	writeCommand( 0x29 );                 // Display on
  cs_high();
}

void ST7796::flush( lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p ) {
  cs_low();
  setWindow( area->x1, area->y1, area->x2, area->y2 );

  for( int16_t y = area->y1; y <= area->y2; y++ ) {
    for( int16_t x = area->x1; x <= area->x2; x++ ) {
      write_16( (*color_p).full );
      color_p++;
    }
  }

  cs_high();
  // IMPORTANT!!!
  // Inform the graphics library that you are ready with the flushing.
  lv_disp_flush_ready( drv );
}

bool ST7796::readTouch( lv_indev_drv_t* drv, lv_indev_data_t* data ) {
  bool valid = true;
  int16_t x = 0;
  int16_t y = 0;

  uint8_t irq = digitalRead( Config::ST7796_TOUCH_IRQ_PIN );
  if( irq == 0 ) {
    x = read_spi( CMD_X_READ );
    y = read_spi( CMD_Y_READ );
    //ESP_LOGI(TAG, "P(%d,%d)", x, y);

    /*Normalize Data back to 12-bits*/
    x = x >> 4;
    y = y >> 4;
    //ESP_LOGI(TAG, "P_norm(%d,%d)", x, y);

    xpt2046_corr( &x, &y );
    xpt2046_avg( &x, &y );
    last_x = x;
    last_y = y;
    //ESP_LOGI(TAG, "x = %d, y = %d", x, y);
  } else {
    x = last_x;
    y = last_y;
    avg_last = 0;
    valid = false;
  }
  data->point.x = x;
  data->point.y = y;
  data->state = valid == false ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;
  return false;                       // No buffering now so no more data read
}

void ST7796::setBrigthness( uint8_t v ) {
//	writeCommand( 0x51 );
//	writeData( v );
  cs_low();
	writeCommand( 0x53 );
	writeData( 0x00 );
  cs_high();
}

// inline void tft_begin_write( void ) {
//   cs_low();
// }

// inline void tft_end_write( void ) {
//   cs_high();
// }

/**/

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"

inline void ST7796::cs_high() {
  if( ST7796_CS_PIN >= 32 ) {
    GPIO.out1_w1ts.val = (1 << (ST7796_CS_PIN - 32));
  } else if( ST7796_CS_PIN >= 0 ) {
    GPIO.out_w1ts = (1 << ST7796_CS_PIN);
  }
}

inline void ST7796::cs_low() {
  if( ST7796_CS_PIN >= 32 ) {
    GPIO.out1_w1tc.val = (1 << (ST7796_CS_PIN - 32));
  } else if( ST7796_CS_PIN >= 0 ) {
    GPIO.out_w1tc = (1 << ST7796_CS_PIN);
  }
}

#pragma GCC diagnostic pop

// Define the DC (TFT Data/Command or Register Select (RS)) pin drive code
inline void ST7796::dc_c() {
  GPIO.out_w1tc = (1 << ST7796_DC_PIN);
}

inline void ST7796::dc_d() {
  GPIO.out_w1ts = (1 << ST7796_DC_PIN);
}

// Define the WR (TFT Write) pin drive code
inline void ST7796::wr_l() {
  GPIO.out_w1tc = (1 << ST7796_WR_PIN);
}

inline void ST7796::wr_h() {
  GPIO.out_w1ts = (1 << ST7796_WR_PIN);
}


//    #define RD_L GPIO.out_w1tc = (1 << TFT_RD)
    //#define RD_L digitalWrite(TFT_WR, LOW)
//    #define RD_H GPIO.out_w1ts = (1 << TFT_RD)
    //#define RD_H digitalWrite(TFT_WR, HIGH)

// void TFT_eSPI::gpioMode(uint8_t gpio, uint8_t mode)
// {
//   if(mode == INPUT) GPIO.enable_w1tc = ((uint32_t)1 << gpio);
//   else GPIO.enable_w1ts = ((uint32_t)1 << gpio);

//   ESP_REG(DR_REG_IO_MUX_BASE + esp32_gpioMux[gpio].reg) // Register lookup
//     = ((uint32_t)2 << FUN_DRV_S)                        // Set drive strength 2
//     | (FUN_IE)                                          // Input enable
//     | ((uint32_t)2 << MCU_SEL_S);                       // Function select 2
//   GPIO.pin[gpio].val = 1;                               // Set pin HIGH
// }

/***************************************************************************************
** Function name:           read byte  - supports class functions
** Description:             Read a byte from ESP32 8 bit data port
***************************************************************************************/
// Parallel bus MUST be set to input before calling this function!
// uint8_t TFT_eSPI::readByte(void)
// {
//   uint8_t b = 0xAA;

// #if defined (TFT_PARALLEL_8_BIT)
//   RD_L;
//   uint32_t reg;           // Read all GPIO pins 0-31
//   reg = gpio_input_get(); // Read three times to allow for bus access time
//   reg = gpio_input_get();
//   reg = gpio_input_get(); // Data should be stable now
//   RD_H;

//   // Check GPIO bits used and build value
//   b  = (((reg>>TFT_D0)&1) << 0);
//   b |= (((reg>>TFT_D1)&1) << 1);
//   b |= (((reg>>TFT_D2)&1) << 2);
//   b |= (((reg>>TFT_D3)&1) << 3);
//   b |= (((reg>>TFT_D4)&1) << 4);
//   b |= (((reg>>TFT_D5)&1) << 5);
//   b |= (((reg>>TFT_D6)&1) << 6);
//   b |= (((reg>>TFT_D7)&1) << 7);
// #endif

//   return b;
// }


// void ST7796::bus_direction( uint32_t mask, uint8_t mode ) {
//   gpioMode(TFT_D0, mode);
//   gpioMode(TFT_D1, mode);
//   gpioMode(TFT_D2, mode);
//   gpioMode(TFT_D3, mode);
//   gpioMode(TFT_D4, mode);
//   gpioMode(TFT_D5, mode);
//   gpioMode(TFT_D6, mode);
//   gpioMode(TFT_D7, mode);
// }

// uint8_t ST7796::read_8( uint8_t c ) {
//   uint8_t reg = 0;
//   writeCommand( c );
//   busDir(dir_mask, INPUT);
//   cs_low();
//   // Read nth parameter (assumes caller discards 1st parameter or points index to 2nd)
//   while(index--) reg = readByte();
//   busDir(dir_mask, OUTPUT);
//   cs_high();
//   return reg;
// }

// uint16_t ST7796::read_16( uint8_t c ) {
//   uint32_t reg;

//   reg  = (readcommand8(cmd_function, index + 0) <<  8);
//   reg |= (readcommand8(cmd_function, index + 1) <<  0);

//   return reg;

// }

int16_t ST7796::read_spi( uint8_t reg ) {
  uint8_t buffer[2];
  int16_t result;

  SPI.beginTransaction( SPISettings( SPI_CLOCK, MSBFIRST, SPI_MODE0 ));
  digitalWrite( Config::ST7796_TOUCH_CS_PIN, LOW );
  buffer[0] = reg;
  SPI.transferBytes( buffer, nullptr, 1 );
  SPI.transferBytes( nullptr, buffer, 2 );
  digitalWrite( Config::ST7796_TOUCH_CS_PIN, HIGH );
  SPI.endTransaction();

  result = (buffer[0] << 8) | buffer[1];
  return result;
}

/**
 * Write 8 bits to TFT
 */
void ST7796::write_8( uint8_t d ) {
  GPIO.out_w1tc = clr_mask;
  GPIO.out_w1ts = xset_mask[d];
  wr_h();
}

/**
 * Write 16 bits to TFT
 */
void ST7796::write_16( uint16_t d ) {
  GPIO.out_w1tc = clr_mask;
  GPIO.out_w1ts = xset_mask[(uint8_t) ((d) >> 8)];
  wr_h();
  GPIO.out_w1tc = clr_mask;
  GPIO.out_w1ts = xset_mask[(uint8_t) ((d) >> 0)];
  wr_h();
}

/**
 * Write two concatenated 16 bit values to TFT
 */
void ST7796::write_32c( uint16_t c, uint16_t d ) {
  GPIO.out_w1tc = clr_mask;
  GPIO.out_w1ts = xset_mask[(uint8_t) ((c) >> 8)];
  wr_h();
  GPIO.out_w1tc = clr_mask;
  GPIO.out_w1ts = xset_mask[(uint8_t) ((c) >> 0)];
  wr_h();
  GPIO.out_w1tc = clr_mask;
  GPIO.out_w1ts = xset_mask[(uint8_t) ((d) >> 8)];
  wr_h();
  GPIO.out_w1tc = clr_mask;
  GPIO.out_w1ts = xset_mask[(uint8_t) ((d) >> 0)];
  wr_h();
}

void ST7796::xpt2046_avg( int16_t* x, int16_t* y ) {
  // Shift out the oldest data
  uint8_t i;
  for( i = Config::ST7796_TOUCH_AVG - 1; i > 0 ; i-- ) {
    avg_buf_x[i] = avg_buf_x[i - 1];
    avg_buf_y[i] = avg_buf_y[i - 1];
  }
  // Insert the new point
  avg_buf_x[0] = *x;
  avg_buf_y[0] = *y;
  if( avg_last < Config::ST7796_TOUCH_AVG ) avg_last++;
  // Sum the x and y coordinates
  int32_t x_sum = 0;
  int32_t y_sum = 0;
  for( i = 0; i < avg_last ; i++ ) {
    x_sum += avg_buf_x[i];
    y_sum += avg_buf_y[i];
  }
  // Normalize the sums
  (*x) = (int32_t)x_sum / avg_last;
  (*y) = (int32_t)y_sum / avg_last;
}

void ST7796::xpt2046_corr( int16_t* x, int16_t* y ) {
  if( Config::ST7796_TOUCH_XY_SWAP ) {
    int16_t swap_tmp;
    swap_tmp = *x;
    *x = *y;
    *y = swap_tmp;
  }

  if( (*x) > ST7796_TOUCH_X_MIN ) {
    (*x) -= ST7796_TOUCH_X_MIN;
  } else {
    (*x) = 0;
  }

  if( (*y) > ST7796_TOUCH_Y_MIN ) {
    (*y) -= ST7796_TOUCH_Y_MIN;
  } else {
    (*y) = 0;
  }

  (*x) = (uint32_t)((uint32_t)(*x) * LV_HOR_RES) / (ST7796_TOUCH_X_MAX - ST7796_TOUCH_X_MIN);
  (*y) = (uint32_t)((uint32_t)(*y) * LV_VER_RES) / (ST7796_TOUCH_Y_MAX - ST7796_TOUCH_Y_MIN);

  if( Config::ST7796_TOUCH_INVERT_X ) {
    (*x) = LV_HOR_RES - (*x);
  }
  if( Config::ST7796_TOUCH_INVERT_Y ) {
    (*y) = LV_VER_RES - (*y);
  }
}

void ST7796::setWindow( int16_t x0, int16_t y0, int16_t x1, int16_t y1 ) {
  dc_c();
  write_8( 0x2A );            // CASET
  dc_d();
  write_32c( x0, x1 );
  dc_c();
  write_8( 0x2B );            // RASET
  dc_d();
  write_32c( y0, y1 );
  dc_c();
  write_8( 0x2C );            // RAMWR
  dc_d();
}

/**
 * Send an 8 bit command to the TFT
 */
void ST7796::writeCommand( uint8_t c ) {
  cs_low();
  dc_c();
  write_8( c );
  dc_d();
  cs_high();
}

/**
 * Send a 8 bit data value to the TFT
 */
void ST7796::writeData( uint8_t d ) {
  cs_low();
  dc_d();                               // Play safe, but should already be in data mode
  write_8( d );
  cs_low();                             // Allow more hold time for low VDI rail
  cs_high();
}
