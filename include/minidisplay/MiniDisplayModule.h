#pragma once
#include "DisplayMenu.h"
#include "DisplaySSD1306.h"
#include "Messages.h"
#include "Module.h"
#include "SimpleMap.h"

using namespace DisplayMenu;

/* MiniDisplayModule */

class MiniDisplayModule : public Module, public Executor {

typedef std::function<void(const String& value)> OnValueCallback;
typedef std::function<void(const String& menuId, const String& entryId, OnValueCallback valueCallback)> OnRequestValueListener;
typedef std::function<void(const String& menuId, const String& entryId, const String& value)> OnChangeValueListener;

typedef union {
  uint8_t data;
  struct {
    uint8_t initialized     : 1;
    uint8_t display_on      : 1;
    uint8_t any_key_pressed : 1;
    uint8_t spare03         : 1;
    uint8_t spare04         : 1;
    uint8_t spare05         : 1;
    uint8_t spare06         : 1;
    uint8_t spare07         : 1;
  };
} StateFlags;

/* MiniDisplayModule */

private:
  Display_SSD1306 display;
  std::vector<Menu*> menuList;
  String defaultMenu;
  String valuesResolveTopic;

  StateFlags flags;
  Menu* activeMenu = nullptr;
  Entry* selectedEntry = nullptr;
  int16_t sleepTimeout;
  bool needRedrawMenu;

  OnRequestValueListener onRequestValueListener = 0;
  OnChangeValueListener onChangeValueListener = 0;

public:
  MiniDisplayModule();
  virtual ~MiniDisplayModule();
  virtual void         tick_100mS( uint8_t phase );
  // Module identification.
  virtual const char*  getId()    { return MINI_DISPLAY_MODULE; }
  virtual const char*  getName()  { return Messages::TITLE_MINI_DISPLAY_MODULE; }
  // Module Web interface
  virtual const String getModuleWebpage();
  // A generic getData/setData interface
  virtual const String getString( const String& key );
  virtual ResultData   setString( const String& key, const String& value );
  // DisplayMenu::Executor interface.
  virtual void         selectEntry( int8_t index );
  virtual void         executeEntryCommand( const String& cmd ) ;
  virtual bool         showMenu( const String& menuId, const String& entryId = "" );
  virtual void         showEntryEditor( bool requestData );
  virtual void         dismissEntryEditor( const String& value );
  // Callbacks
  void                 setOnRequestValueListener( OnRequestValueListener listener )  {onRequestValueListener = listener;}
  void                 setOnChangeValueListener( OnChangeValueListener listener )  {onChangeValueListener = listener;}
  // Public API
  void                 addMenu( Menu* menu ) { menuList.push_back( menu ); }
  void                 setDisplayEnabled( bool value );
  bool                 setEntry( const String& menuId, const String& entryId, const std::function<void(Entry* entry)> f );
  void                 setTemplateParameter( const String& key, const String& value );
  void                 showDefaultMenuEntry();
  void                 redrawMenu();
  bool                 selectMenu( const String& menuId, const String& entryId );
  bool                 selectMenu( const String& entryId );

protected:
  virtual bool         handleCommand( const String& cmd, const String& args );
  virtual void         resolveTemplateKey( const String& key, String& out );

private:
  void                 buildJsonMenu();
  void                 buildDefaultMenu();
  bool                 dispatchRequestedValue( const String& jsonString );
  String               getMenuData();
  uint16_t             getSleepTimeout();
  Entry*               findEntry( const String& menuId, const String& entryId );
  Menu*                findMenu( const String& menuId );
  void                 handleKeyPress( KeyEvent ev );
  void                 redrawEditor();

};