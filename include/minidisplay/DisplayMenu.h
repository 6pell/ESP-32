#pragma once
#include <vector>
#include <WString.h>
#include "DisplaySSD1306.h"

#include <Fonts/FreeSans9pt7b.h>
#include "fonts/Dialog_plain_10.h"

namespace DisplayMenu {

  /* Constants */

  const int MAX_MENU_ID_LENGTH         = 9;                    // Max menu ID length.

                                                               // Constants participated in menu entries drawing.
  const int DISPLAY_WIDTH              = 128;                  // OLED display width, in pixels
  const int DISPLAY_HEIGHT             = 64;                   // OLED display height, in pixels

  const int ENTRY_PADDING              = 4;                    // Paddings around the menu entry
  const GFXfont* const TITLE_FONT      = &FreeSans9pt7b;       // Title font
  const int TITLE_HEIGHT               = 13;                   // Title font height (without paddings)
  const int TITLE_OFFSET_Y             = 12;                   // Vertical Y offset because customs fonts are aligned by baseline.

  const GFXfont* const TEXT_FONT       = &Dialog_plain_10;     // Text font
  const int TEXT_HEIGHT                = 10;
  const int TEXT_OFFSET_Y              = 7;

  /* KeyEvent enum */

  enum KeyEvent { SELECT, UP, DOWN };

  const int8_t NEXT_ENTRY_OR_VALUE     = -1;
  const int8_t PREVIOUS_ENTRY_OR_VALUE = -2;

  /* Executor interface: an interface to perform Entry actions. */

  class Executor {
  public:
    // Menu selection
    virtual void selectEntry( int8_t index ) = 0;
    virtual void executeEntryCommand( const String& cmd ) = 0;
    virtual bool showMenu( const String& menuId, const String& entryId ) = 0;
    // Editor
    virtual void showEntryEditor( bool requestData ) = 0;
    virtual void dismissEntryEditor( const String& value ) = 0;
  };

  /* Menu entries */

  class Entry {
  protected:
    String  id;
    String  title;
    String  text;
    bool    visible;

  public:
    Entry( const String& _id, const String& _title );
    Entry( const String& _id, const String& _title, const String& _text );
    virtual ~Entry() {}

    /**
     * Draws an entry on a display.
     * @param display The reference to display object where the entry should draw itself.
     */
    virtual void draw( Display_SSD1306& display );
    virtual void drawEditor( Display_SSD1306& display ) {;}
    void drawTitleOnTop( Display_SSD1306& display );
    void drawTitleOnBottom( Display_SSD1306& display );

    virtual uint8_t getHeight( Display_SSD1306& display );
    virtual void onKeyPress( const KeyEvent ev, Executor& executor );

    const String getId()                         {return id;}
    const String getTitle()                      {return title;}
    const bool isVisible()                       {return visible;}
    void setTitle( const String& value )         {title = value;}
    void setText( const String& txt )            {text = txt;}
    void setVisible( bool value )                {visible = value;}
  };

  /* Link: a menu entry that represents a link to other menu */

  class Link : public Entry {
  private:
    String targetMenuId;
  public:
    Link( const String& id, const String& title, const String& target )
      : Entry( id, title ), targetMenuId( target ) {}
    Link( const String& _id, const String& _title, const String& _text, const String& _target)
      : Entry( _id, _title ) {
        text = _text;
        targetMenuId = _target;
      }
    virtual ~Link() {}
    virtual void onKeyPress( const KeyEvent ev, Executor& executor );
  };

  /* Command: a menu entry to trigger some command  */

  class Command : public Entry {
  private:
    String command;
  public:
    Command( const String& id, const String& title, const String& cmd )
      : Entry( id, title ), command( cmd ) {}
    virtual ~Command() {}
    virtual void onKeyPress( const KeyEvent ev, Executor& executor );
  };

  /* Editors */

  union EditorFlags {
    uint8_t data;
    struct {
      uint8_t editMode         : 1;     // In edit mode the value is changed with up/down keys.
      uint8_t needMeasureWidth : 1;     // Indicates that it's need to measure the value with max possible text width.
    };

    EditorFlags() : data(0) {}
  };

  class BaseEditor : public Entry {
  protected:
    EditorFlags flags;
  public:
    BaseEditor( const String& id, const String& title ) : Entry( id, title ) {}
    virtual ~BaseEditor()  {}
    virtual void onKeyPress( const KeyEvent ev, Executor& executor );
  protected:
    virtual const String getValueAsString() = 0;
    virtual bool selectValue( int8_t index ) = 0;
  };

  class Number : public BaseEditor {
  private:
    int value = 0;
    int min = 0;
    int max = 0;
    int step = 0;
    uint16_t valueMaxWidth = 20;
  public:
    Number( const String& id, const String& title ) : BaseEditor( id, title ) {}
    virtual ~Number() {}
    virtual void drawEditor( Display_SSD1306& display );
    void setValue( int value, int min, int max, int step );
  protected:
    virtual const String getValueAsString();
    virtual bool selectValue( int8_t index );
  };

  class Choice : public BaseEditor {
  private:
    std::vector<String> options;
    uint8_t selectedPosition;
  public:
    Choice( const String& id, const String& title ) : BaseEditor( id, title ) {}
    virtual ~Choice();
    virtual void drawEditor( Display_SSD1306& display );
    void setValue( std::vector<String>& options, String& selectedKey );
  protected:
    virtual const String getValueAsString();
    virtual bool selectValue( int8_t index );
  };

  /* Menu */

  class Menu {
  private:
    String menuId;
    String title;
    std::vector<Entry*> entries;

  public:
    Menu( const String& id, const String& title );
    Menu( const String& id, const String& title, const std::initializer_list<Entry*> &list );

    const String getId()  {return menuId;}
    const String getTitle()  {return title;}
    void drawTitleOnTop( Display_SSD1306& display );

    void add( Entry* const entry );
    void addAt( Entry* const entry, uint8_t position );
    int count()  {return entries.size();}
    Entry* getEntry( const uint8_t position );
    Entry* getEntry( const String& id );
    Entry* getFirstVisibleEntry();
    Entry* getNextVisibleEntry( const String& id );
    Entry* getPreviousVisibleEntry( const String& id );
    int8_t getPosition( const String& id );
    void remove( const String& id );
    void reserve( int num_entries )  { entries.reserve( num_entries ); }
    void setVisible( const String& entry_id, bool value );
    void setTitle( const String& entry_id, const String& txt );
  };
}