#include <SPI.h>
#include "lvgl/ST7796Driver.h"

/* Public */

ST7796Driver::ST7796Driver() {
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

  if( ST7796_BACKLIGHT_PIN != -1 ) {
    pinMode( ST7796_BACKLIGHT_PIN, OUTPUT );
    setBacklight( true );
  }

  // Set to output once again in case D6 (MISO) is used for CS
  if( ST7796_CS_PIN != -1 ) {
    pinMode( ST7796_CS_PIN, OUTPUT );
    digitalWrite( ST7796_CS_PIN, HIGH );  // Chip select high (inactive)
  }

  pinMode( ST7796_WR_PIN, OUTPUT );
  digitalWrite( ST7796_WR_PIN, HIGH );    // Set write strobe high (inactive)

  // Set to output once again in case D6 (MISO) is used for DC
  if( ST7796_DC_PIN != -1 ) {
    pinMode( ST7796_DC_PIN, OUTPUT );
    digitalWrite( ST7796_DC_PIN, HIGH );  // Data/Command high = data mode
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

  cs_low();
	delay( 120 );
  writeCommandSequence( INIT_COMMANDS );
  cs_high();
  delay( 120 );
  cs_low();

	writeCommand( 0x29 );                 // Display on
  cs_high();

  // Setup touch interface
  pinMode( ST7796_TOUCH_IRQ_PIN, INPUT );
  pinMode( ST7796_TOUCH_CS_PIN, OUTPUT );
}

ST7796Driver::~ST7796Driver() {
  // Do nothing here
}

void ST7796Driver::flushArea( lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p ) {
  cs_low();
  setWindow( area->x1, area->y1, area->x2, area->y2 );

  for( int16_t y = area->y1; y <= area->y2; y++ ) {
    for( int16_t x = area->x1; x <= area->x2; x++ ) {
      write16( (*color_p).full );
      color_p++;
    }
  }

  cs_high();
  // IMPORTANT!!!
  // Inform the graphics library that you are ready with the flushing.
  lv_disp_flush_ready( drv );
}

bool ST7796Driver::readTouch( lv_indev_drv_t* drv, lv_indev_data_t* data ) {
  bool valid = true;
  int16_t x = 0;
  int16_t y = 0;

  uint8_t irq = digitalRead( Config::ST7796_TOUCH_IRQ_PIN );
  if( irq == 0 ) {
    SPI.beginTransaction( SPISettings( TOUCH_SPI_CLOCK, MSBFIRST, SPI_MODE0 ));
    digitalWrite( ST7796_TOUCH_CS_PIN, LOW );
    x = read_spi( TOUCH_CMD_X_READ );
    y = read_spi( TOUCH_CMD_Y_READ );
    digitalWrite( ST7796_TOUCH_CS_PIN, HIGH );
    SPI.endTransaction();

    // Normalize Data back to 12-bits
    x = x >> 4;
    y = y >> 4;

    xpt2046_corr( &x, &y );
    xpt2046_avg( &x, &y );
    last_x = x;
    last_y = y;
    //Log.verbose( "TFT touch (x=%d,y=%d)" CR, x, y );
  } else {
    x = last_x;
    y = last_y;
    avg_last = 0;
    valid = false;
  }
  data->point.x = x;
  data->point.y = y;
  data->state = valid == false ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;

  // No buffering now so no more data read
  return false;
}

void ST7796Driver::setBacklight( bool enable ) {
  if( ST7796_BACKLIGHT_PIN != -1 ) {
    digitalWrite( ST7796_BACKLIGHT_PIN, enable );
  }
}

/* Private */

void ST7796Driver::setWindow( int16_t x0, int16_t y0, int16_t x1, int16_t y1 ) {
  dc_c();
  write8( 0x2A );            // CASET
  dc_d();
  write32c( x0, x1 );
  dc_c();
  write8( 0x2B );            // RASET
  dc_d();
  write32c( y0, y1 );
  dc_c();
  write8( 0x2C );            // RAMWR
  dc_d();
}

/**
 * Send an 8 bit command to the TFT
 */
void ST7796Driver::writeCommand( uint8_t c ) {
  cs_low();
  dc_c();
  write8( c );
  dc_d();
  cs_high();
}

/**
 * Send a 8 bit data value to the TFT
 */
void ST7796Driver::writeData( uint8_t d ) {
  cs_low();
  dc_d();                               // Play safe, but should already be in data mode
  write8( d );
  cs_low();                             // Allow more hold time for low VDI rail
  cs_high();
}

/* Private, display low level methods */

/**
 * Write 8 bits to TFT
 */
void ST7796Driver::write8( uint8_t d ) {
  GPIO.out_w1tc = CLR_MASK;
  GPIO.out_w1ts = xset_mask[d];
  wr_h();
}

/**
 * Write 16 bits to TFT
 */
void ST7796Driver::write16( uint16_t d ) {
  GPIO.out_w1tc = CLR_MASK;
  GPIO.out_w1ts = xset_mask[(uint8_t) ((d) >> 8)];
  wr_h();
  GPIO.out_w1tc = CLR_MASK;
  GPIO.out_w1ts = xset_mask[(uint8_t) ((d) >> 0)];
  wr_h();
}

/**
 * Write two concatenated 16 bit values to TFT
 */
void ST7796Driver::write32c( uint16_t c, uint16_t d ) {
  GPIO.out_w1tc = CLR_MASK;
  GPIO.out_w1ts = xset_mask[(uint8_t) ((c) >> 8)];
  wr_h();
  GPIO.out_w1tc = CLR_MASK;
  GPIO.out_w1ts = xset_mask[(uint8_t) ((c) >> 0)];
  wr_h();
  GPIO.out_w1tc = CLR_MASK;
  GPIO.out_w1ts = xset_mask[(uint8_t) ((d) >> 8)];
  wr_h();
  GPIO.out_w1tc = CLR_MASK;
  GPIO.out_w1ts = xset_mask[(uint8_t) ((d) >> 0)];
  wr_h();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
#pragma GCC diagnostic ignored "-Wshift-count-negative"

void ST7796Driver::cs_high() {
  if( ST7796_CS_PIN >= 32 ) {
    GPIO.out1_w1ts.val = (1 << (ST7796_CS_PIN - 32));
  } else if( ST7796_CS_PIN >= 0 ) {
    GPIO.out_w1ts = (1 << ST7796_CS_PIN);
  }
}

void ST7796Driver::cs_low() {
  if( ST7796_CS_PIN >= 32 ) {
    GPIO.out1_w1tc.val = (1 << (ST7796_CS_PIN - 32));
  } else if( ST7796_CS_PIN >= 0 ) {
    GPIO.out_w1tc = (1 << ST7796_CS_PIN);
  }
}

#pragma GCC diagnostic pop

void ST7796Driver::dc_c() {
  GPIO.out_w1tc = (1 << ST7796_DC_PIN);
}

void ST7796Driver::dc_d() {
  GPIO.out_w1ts = (1 << ST7796_DC_PIN);
}

void ST7796Driver::wr_l() {
  GPIO.out_w1tc = (1 << ST7796_WR_PIN);
}

void ST7796Driver::wr_h() {
  GPIO.out_w1ts = (1 << ST7796_WR_PIN);
}

int16_t ST7796Driver::read_spi( uint8_t reg ) {
  uint8_t buffer[2];
  int16_t result;

  SPI.transfer( reg );
  buffer[0] = SPI.transfer( 0 );
  buffer[1] = SPI.transfer( 0 );

  result = (buffer[0] << 8) | buffer[1];
  return result;
}

void ST7796Driver::xpt2046_avg( int16_t* x, int16_t* y ) {
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

void ST7796Driver::xpt2046_corr( int16_t* x, int16_t* y ) {
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

  if( ST7796_TOUCH_INVERT_X ) {
    (*x) = LV_HOR_RES - (*x);
  }
  if( ST7796_TOUCH_INVERT_Y ) {
    (*y) = LV_VER_RES - (*y);
  }
}
