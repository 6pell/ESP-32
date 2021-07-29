#include <ArduinoLog.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "ModulesManager.h"
#include "MqttClientModule.h"
#include "Options.h"
#include "str_switch.h"
#include "Utils.h"
#include "minidisplay/DisplayMenu.h"
#include "minidisplay/MiniDisplayModule.h"

/* MiniDisplayModule */

using namespace DisplayMenu;

MiniDisplayModule::MiniDisplayModule() : display( DISPLAY_WIDTH, DISPLAY_HEIGHT ) {
  properties.has_module_webpage = true;
  properties.tick_100mS_required = true;
  flags.data = 0;
  flags.display_on = true;
  sleepTimeout = getSleepTimeout();

  pinMode( Config::MINI_DISPLAY_SELECT_PIN, INPUT );
  pinMode( Config::MINI_DISPLAY_UP_PIN, INPUT );
  pinMode( Config::MINI_DISPLAY_DOWN_PIN, INPUT );

  // Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
  flags.initialized = display.begin( SSD1306_SWITCHCAPVCC, 0x3C );
  if( !flags.initialized ) {
    Log.error( "DISP SSD1306 allocation failed" CR );
  } else {
    display.setTextSize( 1 );
    display.setTextColor( WHITE );
    display.setTextWrap( false );
    display.setLeftPadding( ENTRY_PADDING );

    display.setTemplateParameter( "PROJECT", Config::PROJECT_NAME );
    display.setTemplateParameter( "VERSION", Utils::getSystemVersionName() );

    buildJsonMenu();
  }
}

MiniDisplayModule::~MiniDisplayModule() {
  // do nothing
}

const String MiniDisplayModule::getModuleWebpage() {
  return makeWebpage( "/module_mini_display.html" );
}

// A generic getData/setData interface

const String MiniDisplayModule::getString( const String& key ) {
  SWITCH( key.c_str() ) {
    // Get the module export config (JSON string, the menuconfig + template parameters)
    CASE( Config::KEY_EXPORT_CONFIGURATION ):
      return getStringOption( "MenuData", "{}" );
    // Get the menu data (JSON array string).
    CASE( Config::KEY_MENU_DATA ):
      return getMenuData();
    DEFAULT_CASE:
      return Module::getString( key );
  }
}

ResultData MiniDisplayModule::setString( const String& key, const String& value ) {
  SWITCH( key.c_str() ) {
    // Import the module config.
    CASE( Config::KEY_IMPORT_CONFIGURATION ):
      setStringOption( "MenuData", value );
      return RESULT_OK;
    // Permanently store the new menu configuration (the string representation of JSON array).
    CASE( Config::KEY_MENU_DATA ):
      setStringOption( key, value );
      buildJsonMenu();
      return RESULT_OK;
    DEFAULT_CASE:
      return Module::setString( key, value );
  }
}

void MiniDisplayModule::tick_100mS( uint8_t phase ) {
  // Detect keyboard buttons press and release.
  if( !flags.any_key_pressed ) {
    if( digitalRead( Config::MINI_DISPLAY_SELECT_PIN ) == LOW ) {
      flags.any_key_pressed = true;
      handleKeyPress( SELECT );
    }
    else if( digitalRead( Config::MINI_DISPLAY_UP_PIN ) == LOW ) {
      flags.any_key_pressed = true;
      handleKeyPress( UP );
    }
    else if( digitalRead( Config::MINI_DISPLAY_DOWN_PIN ) == LOW ) {
        flags.any_key_pressed = true;
      handleKeyPress( DOWN );
    }
  } else if( digitalRead( Config::MINI_DISPLAY_SELECT_PIN ) == HIGH &&
             digitalRead( Config::MINI_DISPLAY_UP_PIN ) == HIGH &&
             digitalRead( Config::MINI_DISPLAY_DOWN_PIN ) == HIGH ) {
    flags.any_key_pressed = false;
  }
  // Manage the display timeout
  if( flags.display_on && sleepTimeout > 0 ) {
    if( --sleepTimeout == 0 ) {
      setDisplayEnabled( false );
    }
  }
  // Optimized menu redrawing if template parameters was changed
  if( needRedrawMenu ) {
    needRedrawMenu = false;
    redrawMenu();
  }
}

// Executor interface.

void MiniDisplayModule::selectEntry( int8_t index ) {
  if( index == NEXT_ENTRY_OR_VALUE ) {
    if( selectedEntry ) {
      Entry* e = activeMenu->getNextVisibleEntry( selectedEntry->getId() );
      if( e ) {
        selectedEntry = e;
        redrawMenu();
      }
    }
  }
  else if( index == PREVIOUS_ENTRY_OR_VALUE ) {
    if( selectedEntry ) {
      Entry* e = activeMenu->getPreviousVisibleEntry( selectedEntry->getId() );
      if( e ) {
        selectedEntry = e;
        redrawMenu();
      }
    }
  }
}

void MiniDisplayModule::executeEntryCommand( const String& cmd ) {
  // Forward a command to the ModulesManager.
  Modules.dispatchCommand( cmd );
}

bool MiniDisplayModule::showMenu( const String& menuId, const String& entryId ) {
  Menu* m = findMenu( menuId );
  if( !m ) {
    Log.warning( "DISP menu ID '%s' is not found" CR, menuId.c_str() );
  } else {
    Entry* e = m->getEntry( entryId );
    if( !e ) {
      // The entry ID is not found. Get the 1st visible entry as a workaround.
      // Type a warning only when entry ID is not empty, because empty value
      // means to select the 1st visible entry.
      if( entryId.length() > 0 ) {
        Log.warning( "DISP entry ID '%s' is not found" CR, entryId.c_str() );
      }
      e = m->getFirstVisibleEntry();
    }
    if( e ) {
      activeMenu = m;
      selectedEntry = e;
      redrawMenu();
      return true;
    }
  }
  return false;
}

void MiniDisplayModule::showEntryEditor( bool requestData ) {
  if( requestData ) {
    // If callback is set, it's responsive to handle a request of value to be edited.
    // The returned value should be a JSON string.
    if( onRequestValueListener ) {
      onRequestValueListener( activeMenu->getId(), selectedEntry->getId(), [this](const String& value){
        dispatchRequestedValue( value );
      });
    }
    // Otherwise use the MQTT to resolve a value to be edited.
    // The returned value is provided via usual handleCommand() interface.
    // The MQTT topic should be like cmnd/<device topic>/display editvalue <value JSON object>.
    else if( valuesResolveTopic.length() > 0 ) {
      Modules.execute( MQTT_MODULE, [this](Module* module) {
        MqttClientModule* mqtt = (MqttClientModule*) module;
        StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
        json["device"] = mqtt->getDeviceTopic();
        json["menu"] = activeMenu->getId();
        json["entry"] = selectedEntry->getId();
        mqtt->publish( valuesResolveTopic, json.as<String>(), false );
      });
    }
    // Neither callback nor MQTT topic are specified. Can't do anything.
    else {
      selectedEntry->setText( "Can't resolve value" );
      redrawMenu();
    }
  } else {
    // The entry already has data, it's only need to redraw the editor on a display.
    redrawEditor();
  }
}

void MiniDisplayModule::dismissEntryEditor( const String& value ) {
  // If callback is set, it's responsive to handle the modified value.
  // Callback is responsive to manage (store, send, etc..) the modified value.
  if( onChangeValueListener ) {
    StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
    json["menu"] = activeMenu->getId();
    json["entry"] = selectedEntry->getId();
    json["value"] = value;
    onChangeValueListener( activeMenu->getId(), selectedEntry->getId(), json.as<String>() );
  }
  // Otherwise use the MQTT to resolve a value to be edited.
  else if( valuesResolveTopic.length() > 0 ) {
    Modules.execute( MQTT_MODULE, [this,&value](Module* module) {
      MqttClientModule* mqtt = (MqttClientModule*) module;
      // Prepare a JSON string thet represents an edited value.
      StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
      json["device"] = mqtt->getDeviceTopic();
      json["menu"] = activeMenu->getId();
      json["entry"] = selectedEntry->getId();
      json["value"] = value;
      mqtt->publish( valuesResolveTopic, json.as<String>(), false );
    });
  }
  redrawMenu();
}

/* Public */

void MiniDisplayModule::setDisplayEnabled( bool value ) {
  if( value ) {
    sleepTimeout = getSleepTimeout();
    flags.display_on = true;
    display.ssd1306_command( SSD1306_DISPLAYON );
    redrawMenu();
  } else {
    sleepTimeout = 0;
    flags.display_on = false;
    display.ssd1306_command( SSD1306_DISPLAYOFF );
    showDefaultMenuEntry();
  }
}

bool MiniDisplayModule::setEntry( const String& menuId, const String& entryId, const std::function<void(Entry* entry)> func ) {
  Entry* e = findEntry( menuId, entryId );
  if( e ) {
    func( e );
    return true;
  }
  return false;
}

void MiniDisplayModule::setTemplateParameter( const String& key, const String& value ) {
  display.setTemplateParameter( key, value );
  needRedrawMenu = true;
}

void MiniDisplayModule::showDefaultMenuEntry() {
  auto pair = Utils::split( defaultMenu, '/' );
  bool shown = showMenu( pair.first, pair.second );
  // If the default menu isn't defined, show the most 1st menu in the list.
  if( !shown && menuList.size() ) {
    Menu* menu = menuList[0];
    showMenu( menu->getId() );
  }
}

void MiniDisplayModule::redrawMenu() {
  if( !flags.display_on ) return;

  display.clearDisplay();
  if( selectedEntry ) {
    // If the current position is a 1st visible, draw the menu title on top.
    // Otherwise draw a previous entry title on top.
    Entry* e = activeMenu->getPreviousVisibleEntry( selectedEntry->getId() );
    if( e ) {
      e->drawTitleOnTop( display );
    } else {
      activeMenu->drawTitleOnTop( display );
    }
    // Draw the menu entry at current position.
    selectedEntry->draw( display );
    // if the current position is not a last visible, draw the next entry title on bottom.
    // Otherwise draw nothing.
    e = activeMenu->getNextVisibleEntry( selectedEntry->getId() );
    if( e ) {
      e->drawTitleOnBottom( display );
    }
  }
  display.display();
}

bool MiniDisplayModule::selectMenu( const String& menuId, const String& entryId ) {
  auto menu = findMenu( menuId );
  if( menu ) {
    auto entry = entryId.length() > 0 ? findEntry( menuId, entryId ) : menu->getFirstVisibleEntry();
    if( entry ) {
      // Select the requested menu and entry.
      activeMenu = menu;
      selectedEntry = entry;
      redrawMenu();
      // Turn on the display, if needed.
      sleepTimeout = getSleepTimeout();
      if( !flags.display_on ) {
        setDisplayEnabled( true );
      }
      return true;
    }
  }
  return false;
}

bool MiniDisplayModule::selectMenu( const String& entryId ) {
  for( std::vector<Menu*>::iterator it = menuList.begin(); it != menuList.end(); ++it ) {
    Menu* menu = *it;
    Entry* entry = menu->getEntry( entryId );
    if( entry ) {
      // Select the requested menu and entry.
      activeMenu = menu;
      selectedEntry = entry;
      redrawMenu();
      // Turn on the display, if needed.
      sleepTimeout = getSleepTimeout();
      if( !flags.display_on ) {
        setDisplayEnabled( true );
      }
      return true;
    }
  }
  return false;
}

/* Protected */

bool MiniDisplayModule::handleCommand( const String& cmd, const String& args ) {
  SWITCH( cmd.c_str() ) {
    // ==========================================
    CASE( "disable" ):
      //TODO Disable the menu entry.
      return true;
    // ==========================================
    CASE( "enable" ):
      //TODO Enable the menu entry.
      return true;
    // ==========================================
    // Activate a value editor with provided value data JSON.
    CASE( "editvalue" ): {
      bool rc = dispatchRequestedValue( args );
      handleCommandResults( cmd, args, rc ? Messages::OK : Messages::UNKNOWN_MENU_ENTRY_ID );
      return true;
    }
    // ==========================================
    // Turn on the display.
    CASE( "on" ):
      setDisplayEnabled( true );
      handleCommandResults( cmd, args, Messages::OK );
      return true;
    // ==========================================
    // Turn off the display.
    CASE( "off" ):
      setDisplayEnabled( false );
      handleCommandResults( cmd, args, Messages::OK );
      return true;
    // ==========================================
    // Select some menu entry.
    // Example: "display select root/status"
    CASE( "select" ): {
      // Split the payload into menu ID and entry ID.
      auto pair = Utils::split( args, '/' );
      bool rc = selectMenu( pair.first, pair.second );
      handleCommandResults( cmd, args, rc ? Messages::OK : Messages::UNKNOWN_MENU_ENTRY_ID );
      return true;
    }
    // ==========================================
    // Apply a new text to some menu entry.
    // Example: "display text root/status Status\nAccess point mode\n192.168.4.1"
    // Where: <root> is the menu ID, <status> is the entry ID.
    CASE( "text" ): {
      // Split the payload into menu/entry IDs and text.
      auto entry = Utils::split( args );
      String text = entry.second;
      text.replace( "\\n", "\n" );
      // Split the menu/entry IDs.
      auto menu_ids = Utils::split( entry.first, '/' );
      String menuId = menu_ids.first;
      String entryId = menu_ids.second;
      // Apply the text to the menu.
      bool rc = setEntry( menuId, entryId, [&](Entry* entry) {
        entry->setText( text );
        // Redraw the menu when necessary.
        if( activeMenu && selectedEntry ) {
          if( activeMenu->getId() == menuId && selectedEntry->getId() == entryId ) {
            redrawMenu();
          }
        }
      });
      handleCommandResults( cmd, args, rc ? Messages::OK : Messages::UNKNOWN_MENU_ENTRY_ID );
      return true;
    }
    // ==========================================
    // Change the display auto turn off timeout.
    CASE( "timeout" ):
      if( args.length() > 0 ) {
        int16_t value = args.toInt();
        if( value >= 0 ) {
          Options::setShort( getId(), "MenuTimeout", value );
        }
      }
      handleCommandResults( cmd, args, String( getSleepTimeout() ));
      return true;
    // ==========================================
    // Apply a new text to some menu entry.
    // Example: "display title root/status Status OK"
    // Where: <root> is the menu ID, <status> is the entry ID.
    CASE( "title" ): {
      // Split the payload into menu/entry IDs and title.
      auto entry = Utils::split( args );
      String title = entry.second;
      // Split the menu/entry IDs.
      auto menu_ids = Utils::split( entry.first, '/' );
      String menuId = menu_ids.first;
      String entryId = menu_ids.second;
      // Apply the text to the menu.
      bool rc = setEntry( menuId, entryId, [&](Entry* entry) {
        entry->setTitle( title );
        // Redraw the menu when necessary.
        if( activeMenu && activeMenu->getId() == menuId ) {
          redrawMenu();
        }
      });
      handleCommandResults( cmd, args, rc ? Messages::OK : Messages::UNKNOWN_MENU_ENTRY_ID );
      return true;
    }
  }
  return false;
}

void MiniDisplayModule::resolveTemplateKey( const String& key, String& out ) {
  SWITCH( key.c_str() ) {
    // ==========================================
    // Module template parameters
    CASE( "ID" ):
      out += getId();
      break;
    CASE( "Title" ):
      out += Utils::formatModuleSettingsTitle( getId(), getName() );
      break;
  }
}

/* Private */

// Note: The ArduinoJson .as<char*>() cast is used to avoid applying "null" value to empty strings.
void MiniDisplayModule::buildJsonMenu() {
  DynamicJsonDocument doc( Config::JSON_CONFIG_SIZE * 2 );
  DeserializationError rc = deserializeJson( doc, getMenuData() );
  // If the menu JSON cannot be decoded.
  if( rc != DeserializationError::Ok ) {
    Log.error( "DISP Menu decode error. %s" CR, rc.c_str() );
    buildDefaultMenu();
    showDefaultMenuEntry();
    return;
  }
  // Read the menu options.
  JsonObject json = doc.as<JsonObject>();
  defaultMenu = json["default"].as<char*>();
  valuesResolveTopic = json["topic"].as<char*>();
  // Check if the menu is not empty.
  JsonArray menuArray = json["menu"].as<JsonArray>();
  const int count =  menuArray.size();
  if( count == 0 ) {
    Log.notice( "DISP The menu config is empty, default one is used" CR );
    buildDefaultMenu();
    showDefaultMenuEntry();
    return;
  }
  // Build the menu from JSON.
  menuList.clear();
  Menu* currentMenu = nullptr;
  for( JsonObject item : menuArray ) {
    // The menu or entry JSON object must have the "id" and "type" fields.
    // Also they may have "title", "text" and "payload" fields.
    const String type = item["type"].as<char*>();
    const String id = item["id"].as<char*>();
    const String title = item["title"].as<char*>();
    SWITCH( type.c_str() ) {
      CASE( "menu" ): {
        currentMenu = new Menu( id, title );
        addMenu( currentMenu );
        break;
      }
      CASE( "entry" ): {
        const String text = item["text"].as<char*>();
        Entry* entry = new Entry( id, title );
        entry->setText( text );
        if( currentMenu ) {
          currentMenu->add( entry );
        }
        break;
      }
      CASE( "cmd" ): {
        const String text = item["text"].as<char*>();
        const String payload = item["payload"].as<char*>();
        Command* cmd = new Command( id, title, payload );
        cmd->setText( text );
        if( currentMenu ) {
          currentMenu->add( cmd );
        }
        break;
      }
      CASE( "link" ): {
        const String text = item["text"].as<char*>();
        const String payload = item["target"].as<char*>();
        Link* link = new Link( id, title, payload );
        link->setText( text );
        if( currentMenu ) {
          currentMenu->add( link );
        }
        break;
      }
      CASE( "number" ): {
        const String text = item["text"].as<char*>();
        Number* editor = new Number( id, title );
        editor->setText( text );
        if( currentMenu ) {
          currentMenu->add( editor );
        }
        break;
      }
    }
  }
  // Show the menu.
  if( menuList.size() == 0 ) {
    Log.notice( "DISP The menu config is invalid, default one is used" CR );
    buildDefaultMenu();
  }
  showDefaultMenuEntry();
}

void MiniDisplayModule::buildDefaultMenu() {
  menuList.clear();
  addMenu(
    new Menu  {"root",   "", {
    new Entry {"status", "",      "%STATUS%"},
    new Link  {"about",  "About", "about"   }
  }});
  addMenu(
    new Menu  {"about",  "About", {
    new Link  {"info",   "",      "%PROJECT%\nVersion %VERSION%", "root/about"}
  }});
}

bool MiniDisplayModule::dispatchRequestedValue( const String& jsonString ) {
  StaticJsonDocument<Config::JSON_MESSAGE_SIZE> json;
  DeserializationError rc = deserializeJson( json, jsonString );
  if( rc == DeserializationError::Ok &&
      json["menu"] == activeMenu->getId() &&
      json["entry"] == selectedEntry->getId() ) {
    const String type = json["type"];
    SWITCH( type.c_str() ) {
      // Provide a value to the Number editor.
      CASE( "number" ): {
        const int value = json["value"];
        const int min = json["min"];
        const int max = json["max"];
        const int step = json["step"];
        if( Number* editor = dynamic_cast<Number*>(selectedEntry) ) {
          editor->setValue( value, min, max, step );
          redrawEditor();
          return true;
        }
        break;
      }
      // Provide a value to the Choice editor.
      CASE( "choice" ):
        break;
    }
  }
  return false;
}

String MiniDisplayModule::getMenuData() {
  return getStringOption( "MenuData", "{}" );
}

/**
 * Find the menu entry by ID.
 */
Entry* MiniDisplayModule::findEntry( const String& menuId, const String& entryId ) {
  Menu* m = findMenu( menuId );
  if( m ) {
    Entry* e = m->getEntry( entryId );
    if( e ) return e;
  }
  return nullptr;
}

/**
 * Find the menu by ID.
 */
Menu* MiniDisplayModule::findMenu( const String& menuId ) {
  for( std::vector<Menu*>::iterator it = menuList.begin(); it != menuList.end(); ++it ) {
    Menu* menu = *it;
    if( menu->getId() == menuId )
      return menu;
  }
  return nullptr;
}

uint16_t MiniDisplayModule::getSleepTimeout() {
  return getShortOption( "Timeout", Config::MINI_DISPLAY_TIMEOUT * 10 );
}

void MiniDisplayModule::handleKeyPress( KeyEvent ev ) {
  // If display was turned off, the 1st keypress should only wakeup it.
  sleepTimeout = getSleepTimeout();
  if( !flags.display_on ) {
    setDisplayEnabled( true );
  }
  // Handle a keypress.
  else if( activeMenu && selectedEntry ) {
    selectedEntry->onKeyPress( ev, *this );
  }
}

void MiniDisplayModule::redrawEditor() {
  display.clearDisplay();
  selectedEntry->drawEditor( display );
  display.display();
}
