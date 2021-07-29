#include <SPI.h>
#include "lvgl/ST7796Driver.h"
#include <SD.h>
#include "ST7796Module.h"
#include "str_switch.h"
#include "Utils.h"
#include "SDUtils.h"

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Log.verbose("Listing directory: %s" CR, dirname);

    File root = fs.open(dirname);
    if(!root){
        Log.error("Failed to open directory" CR);
        return;
    }
    if(!root.isDirectory()){
        Log.verbose("Not a directory" CR);
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Log.verbose("Creating Dir: %s" CR, path);
    if(fs.mkdir(path)){
        Log.verbose("Dir created");
    } else {
        Log.error("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Log.verbose("Removing Dir: %s" CR, path);
    if(fs.rmdir(path)){
        Log.verbose("Dir removed" CR);
    } else {
        Log.error("rmdir failed" CR);
    }
}

void readFile(fs::FS &fs, const char * path){
    Log.verbose("Reading file: %s" CR, path);

    File file = fs.open(path);
    if(!file){
        Log.error( "Failed to open file for reading" CR );
        return;
    }

    Log.verbose("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Log.verbose("Writing file: %s" CR, path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Log.error( "Failed to open file for writing" CR );
        return;
    }
    if(file.print(message)){
        Log.verbose( "File written" CR );
    } else {
        Log.error( "Write failed" CR );
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Log.verbose("Appending to file: %s" CR, path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Log.error("Failed to open file for appending" CR );
        return;
    }
    if(file.print(message)){
        Log.verbose( "Message appended" CR );
    } else {
        Log.error( "Append failed" CR );
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Log.verbose("Renaming file %s to %s" CR, path1, path2);
    if (fs.rename(path1, path2)) {
        Log.verbose( "File renamed" CR );
    } else {
        Log.error( "Rename failed" CR );
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Log.verbose("Deleting file: %s" CR, path);
    if(fs.remove(path)){
        Log.verbose( "File deleted" CR );
    } else {
        Log.error( "Delete failed" CR );
    }
}

void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Log.verbose("%u bytes read for %u ms" CR, flen, end);
        file.close();
    } else {
        Log.error( "Failed to open file for reading" CR );
    }


    file = fs.open(path, FILE_WRITE);
    if(!file){
        Log.error( "Failed to open file for writing" CR );
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Log.verbose("%u bytes written for %u ms" CR, 2048 * 512, end);
    file.close();
}


DisplayDriver* ST7796Module::driver = nullptr;

ST7796Module::ST7796Module() {
//  properties.tick_100mS_required = true;

  pinMode( ST7796_TOUCH_CS_PIN, OUTPUT );
  digitalWrite( ST7796_TOUCH_CS_PIN, HIGH );
  pinMode( ST7796_SDCARD_CS_PIN, OUTPUT );
  digitalWrite( ST7796_SDCARD_CS_PIN, HIGH );

  // Initialize the SPI interface (ESP32 VSPI) to operate with touchscreen and micro SD.
  SPI.begin();

  // Mounting SD card, it's used to hold graphical assets.
  if( !SD.begin( Config::ST7796_SDCARD_CS_PIN, SPI )) {
    Log.error( "ST7796 SD card mount failed" CR );
  } else {
    const String cardType = SDUtils::toString( SD.cardType() );
    const String cardSize = Utils::toString( SD.cardSize() / (1024 * 1024) );
    Log.verbose( "ST7796 %s card is mounted, size: %sMB" CR, cardType.c_str(), cardSize.c_str() );
  }

  //  1. Call lv_init().
  lv_init();
  //  2. Initialize your drivers.
  driver = new ST7796Driver();
  //  3. Register the display driver in LVGL.
  lv_disp_buf_init( &bufferInfo, &displayBuffer1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX / 10 );
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init( &disp_drv );
  disp_drv.flush_cb = flushDisplay;
  disp_drv.buffer = &bufferInfo;
  lv_disp_drv_register( &disp_drv );

  // 4. Register the touchscreen driver in LVGL.
  /*
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init( &indev_drv );
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = readDisplayTouch;
  lv_indev_t* my_indev = lv_indev_drv_register( &indev_drv );
  */

  //  5. Call lv_tick_inc(x) in every x milliseconds in an interrupt to tell the elapsed time.
  //  6. Call lv_task_handler() periodically in every few milliseconds to handle LVGL related tasks.
  ticker_1ms.attach_ms( 1, ticker_1ms_callback, this );
  ticker_20ms.attach_ms( 20, ticker_20ms_callback, this );

  // /* use a pretty small demo for monochrome displays */
  // /* Get the current screen  */
  // lv_obj_t * scr = lv_disp_get_scr_act(NULL);
  // /* Create a Label on the currently active screen */
  // lv_obj_t * label1 =  lv_label_create(scr, NULL);
  // /* Modify the Label's text */
  // lv_label_set_text(label1, "Hello\nworld!");
  // /* Align the Label to the center
  //  * NULL means align on parent (which is the screen now)
  //  * 0, 0 at the end means an x, y offset after alignment*/
  // lv_obj_align(label1, NULL, LV_ALIGN_CENTER, 0, 0);

  demo_create();


  listDir(SD, "/", 0);
  createDir(SD, "/mydir");
  listDir(SD, "/", 0);
  removeDir(SD, "/mydir");
  listDir(SD, "/", 2);
  writeFile(SD, "/hello.txt", "Hello ");
  appendFile(SD, "/hello.txt", "World!\n");
  readFile(SD, "/hello.txt");
  deleteFile(SD, "/foo.txt");
  renameFile(SD, "/hello.txt", "/foo.txt");
  readFile(SD, "/foo.txt");
  testFileIO(SD, "/test.txt");
}

ST7796Module::~ST7796Module() {
  ticker_20ms.detach();
  ticker_1ms.detach();
  SPI.end();
  delete driver;
}

// void ST7796Module::tick_100mS( uint8_t phase ) {
//   lv_task_handler();
// }

/* Protected */

bool ST7796Module::handleCommand( const String& cmd, const String& args ) {
  SWITCH( cmd.c_str() ) {
    // ==========================================
    // LED backlight
    CASE( "bright" ): {
      uint8_t v = Utils::toByte( args.c_str() );
      driver->setBacklight( v );
      handleCommandResults( cmd, args, Messages::OK );
      return true;
    }

    // ==========================================
    // CASE( "write" ): {
    //   auto p = Utils::split( args );
    //   uint8_t cmd = strtol( p.first.c_str(), NULL, 16 );
    //   uint8_t data = strtol( p.second.c_str(), NULL, 16 );
    //   ST7796::writeCommand( cmd );
    //   ST7796::writeData( data );
    //   return true;
    // }

    // ==========================================
    // CASE( "read" ): {
    //   auto p = Utils::split( args );
    //   uint8_t cmd = strtol( p.first.c_str(), NULL, 16 );
    //   uint8_t data = strtol( p.second.c_str(), NULL, 16 );
    //   ST7796::writeCommand( cmd );
    //   ST7796::writeData( data );
    //   return true;
    // }

    // ==========================================
    // Unknown command
    DEFAULT_CASE:
      return false;
  }
}


/* Private */

static void write_create(lv_obj_t * parent);
static void list_create(lv_obj_t * parent);
static void chart_create(lv_obj_t * parent);

void ST7796Module::demo_create(void) {
  lv_coord_t hres = lv_disp_get_hor_res(NULL);
  lv_coord_t vres = lv_disp_get_ver_res(NULL);

#if LV_DEMO_WALLPAPER
    lv_obj_t * wp = lv_img_create(lv_disp_get_scr_act(NULL), NULL);
    lv_img_set_src(wp, &img_bubble_pattern);
    lv_obj_set_width(wp, hres * 4);
    lv_obj_set_protect(wp, LV_PROTECT_POS);
#endif

  lv_obj_t * tv = lv_tabview_create(lv_disp_get_scr_act(NULL), NULL);
  lv_obj_set_size(tv, hres, vres);

#if LV_DEMO_WALLPAPER
  lv_obj_set_parent(wp, ((lv_tabview_ext_t *) tv->ext_attr)->content);
  lv_obj_set_pos(wp, 0, -5);
#endif

  lv_obj_t * tab1 = lv_tabview_add_tab(tv, "Write");
  lv_obj_t * tab2 = lv_tabview_add_tab(tv, "List");
  lv_obj_t * tab3 = lv_tabview_add_tab(tv, "Chart");

#if LV_DEMO_WALLPAPER == 0
  /*Blue bg instead of wallpaper*/
  //  lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BG, &style_tv_btn_bg);
#endif
  // lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_BG, &style_tv_btn_bg);
  // lv_tabview_set_style(tv, LV_TABVIEW_STYLE_INDIC, &lv_style_plain);
  // lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_REL, &style_tv_btn_rel);
  // lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_PR, &style_tv_btn_pr);
  // lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_TGL_REL, &style_tv_btn_rel);
  // lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_TGL_PR, &style_tv_btn_pr);

//  write_create(tab1);
//  list_create(tab2);
//  chart_create(tab3);

#if LV_DEMO_SLIDE_SHOW
  lv_task_create(tab_switcher, 3000, LV_TASK_PRIO_MID, tv);
#endif
}
