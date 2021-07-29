#include <ArduinoLog.h>
#include "Config.h"
#include "Utils.h"
#include "minidisplay/DisplayMenu.h"

using namespace DisplayMenu;

/* Menu entry (the base class) */

Entry::Entry( const String& _id, const String& _title ) {
  id.reserve( MAX_MENU_ID_LENGTH );
  id = _id;
  title = _title;
  visible = true;
}

Entry::Entry( const String& _id, const String& _title, const String& _text ) {
  id.reserve( MAX_MENU_ID_LENGTH );
  id = _id;
  title = _title;
  text = _text;
  visible = true;
}

/**
 * Draw an entry title + text on center. The title is drawn using a big font,
 * and the text - using a small one. The outer rectangle is always drawn to
 * indicate that it's an active entry.
 */
void Entry::draw( Display_SSD1306& display ) {
  bool draw_title = title.length() > 0 && title.charAt(0) != '~';
  bool draw_text = text.length() > 0;
  // Draw the title.
  if( draw_title ) {
    uint16_t h = TEXT_HEIGHT + ENTRY_PADDING + TITLE_OFFSET_Y;
    if( !draw_text ) h += (DISPLAY_HEIGHT - TEXT_HEIGHT*2) / 4;
    display.setFont( TITLE_FONT );
    display.setCursor( ENTRY_PADDING, h );
    display.printLine( title );
  }
  // Draw the text.
  if( draw_text ) {
    uint16_t h = TEXT_HEIGHT + ENTRY_PADDING + TEXT_OFFSET_Y;
    if( draw_title ) h += TITLE_HEIGHT + 3;
    display.setFont( TEXT_FONT );
    display.setCursor( ENTRY_PADDING, h );
    display.printLine( text );
  }
  // Draw a rectangular cursor around.
  display.drawRect( 0, TEXT_HEIGHT, DISPLAY_WIDTH, DISPLAY_HEIGHT - TEXT_HEIGHT*2 - 1, WHITE );
}

/**
 * Draw an entry title on bottom using a small font.
 */
void Entry::drawTitleOnTop( Display_SSD1306& display ) {
  display.setFont( TEXT_FONT );
  display.setCursor( ENTRY_PADDING, TEXT_OFFSET_Y );
  if( !title.isEmpty() ) {
    display.printLine( title );
  } else {
    display.printLine( id );
  }
}

/**
 * Draw an entry title on bottom using a small font.
 */
void Entry::drawTitleOnBottom( Display_SSD1306& display ) {
  display.setFont( TEXT_FONT );
  display.setCursor( ENTRY_PADDING, DISPLAY_HEIGHT-2 );
  if( !title.isEmpty() ) {
    display.printLine( title );
  } else {
    display.printLine( id );
  }
}

uint8_t Entry::getHeight( Display_SSD1306& display ) {
  uint8_t height = ENTRY_PADDING + ENTRY_PADDING;
  if( title.length() > 0 ) {
    height += TITLE_HEIGHT;
  }
  if( text.length() > 0 ) {
    int16_t  x1, y1;
    uint16_t w, h;
    display.getTextBounds( text, 0, 0, &x1, &y1, &w, &h );
    height += h;
  }
  return height;
}

void Entry::onKeyPress( const KeyEvent ev, Executor& executor ) {
  switch( ev ) {
    case UP:
      executor.selectEntry( PREVIOUS_ENTRY_OR_VALUE );
      break;
    case DOWN:
      executor.selectEntry( NEXT_ENTRY_OR_VALUE );
      break;
    default:
      break;
  }
}

/* Command menu entry */

void Command::onKeyPress( const KeyEvent ev, Executor& executor ) {
  if( ev == SELECT ) {
    executor.executeEntryCommand( command );
  } else {
    // Parent class handles UP and DOWN events.
    Entry::onKeyPress( ev, executor );
  }
}

/* Link menu entry */

void Link::onKeyPress( const KeyEvent ev, Executor& executor ) {
  if( ev == SELECT ) {
    // Try to split the target into menu ID and entry ID. It's also fine to have an empty entry ID.
    auto pair = Utils::split( targetMenuId, '/' );
    executor.showMenu( pair.first, pair.second );
  } else {
    // Parent class handles UP and DOWN events.
    Entry::onKeyPress( ev, executor );
  }
}

/* BaseEditor menu entry */

void BaseEditor::onKeyPress( const KeyEvent ev, Executor& executor ) {
  if( flags.editMode ) {
    switch( ev ) {
      case UP:
        if( selectValue( PREVIOUS_ENTRY_OR_VALUE )) {
          executor.showEntryEditor( false );
        }
        break;
      case DOWN:
        if( selectValue( NEXT_ENTRY_OR_VALUE )) {
          executor.showEntryEditor( false );
        }
        break;
      case SELECT:
        executor.dismissEntryEditor( getValueAsString() );
        flags.editMode = false;
        break;
    }
  } else {
    switch( ev ) {
      case UP:
        executor.selectEntry( PREVIOUS_ENTRY_OR_VALUE );
        break;
      case DOWN:
        executor.selectEntry( NEXT_ENTRY_OR_VALUE );
        break;
      case SELECT:
        executor.showEntryEditor( true );
        break;
    }
  }
}

/* Number editor menu entry */

void Number::drawEditor( Display_SSD1306& display ) {
  drawTitleOnTop( display );

  // Calc the max width of value that can be drawn.
  display.setFont( TITLE_FONT );
  if( flags.needMeasureWidth ) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds( String( max ), 0, 0, &x1, &y1, &w, &h );
    valueMaxWidth = w + ENTRY_PADDING * 3;
    flags.needMeasureWidth = false;
  }

  uint16_t x = DISPLAY_WIDTH - valueMaxWidth - ENTRY_PADDING * 2;
  uint16_t y = TEXT_HEIGHT;
  // Draw the previous value.
  if( (value - step) >= min ) {
    y += ENTRY_PADDING + TITLE_OFFSET_Y;
    display.setCursor( x + ENTRY_PADDING, y );
    display.printLine( String( value - step ));
  } else {
    y += ENTRY_PADDING + TITLE_HEIGHT;
  }

  // Draw a rectangular cursor around.
  y += ENTRY_PADDING + 1;
  display.drawRect( x, y-2, valueMaxWidth, TITLE_HEIGHT+2+2, WHITE );

  // Draw the current value.
  y += TITLE_OFFSET_Y;
  display.setCursor( x + ENTRY_PADDING, y );
  display.printLine( String( value ));

  // Draw the next value
  if( (value + step) <= max ) {
    y += ENTRY_PADDING + TITLE_OFFSET_Y + 1;
    display.setCursor( x + ENTRY_PADDING, y );
    display.printLine( String( value + step ));
  }
}

void Number::setValue( int value, int min, int max, int step ) {
  this->value = value;
  this->min = min;
  this->max = max;
  this->step = step;
  flags.needMeasureWidth = true;
  flags.editMode = true;
}

const String Number::getValueAsString() {
  return String( value );
}

bool Number::selectValue( int8_t index ) {
  if( index == NEXT_ENTRY_OR_VALUE ) {
    if( value < max ) {
      value += step;
      return true;
    }
  }
  else if( index == PREVIOUS_ENTRY_OR_VALUE ) {
    if( value > min ) {
      value -= step;
      return true;
    }
  }
  return false;
}

/* Choice editor menu entry */

void Choice::drawEditor( Display_SSD1306& display ) {
  //TODO
}

void Choice::setValue( std::vector<String>& options, String& selectedKey ) {
  this->options = options;
  // Find the selected option, because it's need to obtain the selected option.
  auto it = std::find_if( options.begin(), options.end(), [&selectedKey](String& kv) {
    auto pair = Utils::split( kv, '/' );
    return selectedKey == pair.first;
  });
  if( it != options.end() ) {
    selectedPosition = std::distance( options.begin(), it );
  } else {
    selectedPosition = 0;
  }
}

const String Choice::getValueAsString() {
  String option = options.at( selectedPosition );
  auto pair = Utils::split( option, '/' );
  return pair.first;
}

bool Choice::selectValue( int8_t index ) {
  if( index == NEXT_ENTRY_OR_VALUE ) {
    //TODO
  }
  else if( index == PREVIOUS_ENTRY_OR_VALUE ) {
    //TODO
  }
  return false;
}

/* Menu */

Menu::Menu( const String& id, const String& title ) {
  this->menuId = id;
  this->title = title;
}

Menu::Menu( const String& id, const String& title, const std::initializer_list<Entry*> &list )
  : Menu( id, title ) {
  for( auto x : list ) {
    entries.push_back( x );
  }
}

/**
 * The menu title is always drawn at top using a small font.
 */
void Menu::drawTitleOnTop( Display_SSD1306& display ) {
  display.setFont( TEXT_FONT );
  display.setCursor( ENTRY_PADDING, TEXT_OFFSET_Y );
  display.printLine( title );
}

/* Menu entries management */

void Menu::add( Entry* const entry ) {
  entries.push_back( entry );
}

void Menu::addAt( Entry* const entry, uint8_t position ) {
  if( position >= 0 && position <= entries.size() ) {
    entries.insert( entries.begin() + position, entry );
  }
}

Entry* Menu::getEntry( const uint8_t position ) {
  if( position >= 0 && position <= entries.size() ) {
    return entries.at( position );
  }
  return NULL;
}

Entry* Menu::getEntry( const String& id ) {
  auto it = std::find_if( entries.begin(), entries.end(), [&id](Entry* en) {
    return en->getId() == id;
  });
  return it != entries.end() ? *it : nullptr;
}

Entry* Menu::getFirstVisibleEntry() {
  for( std::vector<Entry*>::iterator it = entries.begin(); it != entries.end(); ++it ) {
    Entry* entry = *it;
    if( entry->isVisible() )
      return entry;
  }
  return NULL;
}

Entry* Menu::getNextVisibleEntry( const String& id ) {
  int8_t pos = getPosition( id );
  if( pos != -1 ) {
    int8_t size  = entries.size();
    while( ++pos < size ) {
      Entry* e = entries.at( pos );
      if( e->isVisible() )
        return e;
    }
  }
  return NULL;
}

Entry* Menu::getPreviousVisibleEntry( const String& id ) {
  int8_t pos = getPosition( id );
  if( pos > 0 ) {
    while( --pos >= 0 ) {
      Entry* e = entries.at( pos );
      if( e->isVisible() )
        return e;
    }
  }
  return NULL;
}

int8_t Menu::getPosition( const String& id ) {
  auto it = std::find_if( entries.begin(), entries.end(), [&id](Entry* en) {
    return en->getId() == id;
  });
  return it != entries.end() ? std::distance( entries.begin(), it ) : -1;
}

void Menu::remove( const String& id ) {
  auto it = std::find_if( entries.begin(), entries.end(), [&id](Entry* en) {
    return en->getId() == id;
  });
  if( it != entries.end() ) {
    Entry* entry = *it;
    entries.erase( it );
    delete entry;
  }
}

void Menu::setVisible( const String& entry_id, bool value ) {
  Entry* entry = getEntry( entry_id );
  if( entry ) {
    entry->setVisible( value );
  }
}

void Menu::setTitle( const String& entry_id, const String& txt ) {
  Entry* entry = getEntry( entry_id );
  if( entry ) {
    entry->setTitle( txt );
  }
}
