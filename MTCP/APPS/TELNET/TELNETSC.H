/*

   mTCP TelnetSc.h
   Copyright (C) 2009-2022 Michael B. Brutman (mbbrutman@gmail.com)
   mTCP web page: http://www.brutman.com/mTCP


   This file is part of mTCP.

   mTCP is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   mTCP is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with mTCP.  If not, see <http://www.gnu.org/licenses/>.


   Description: Data structures for telnet screen handling code

   Changes:

   2013-03-15: Change cursor handling at right margin to mimic putty
   2011-05-27: Initial release as open source software
   2015-01-22: Add 132 column support
   2018-10-27: Add code to prevent CGA snow
   2020-05-17: Extended keyboard support
   2020-06-13: Alternate code page support.
   2021-10-13: Added printer support.

*/



#ifndef _TELNETSC_H
#define _TELNETSC_H

#define MAX_SELECTION_SIZE 2000
#define MAX_SELECTION_SIZE_PLUS 2048
#define NUMBER_OF_DEFAULT_UTF_TRANSLATIONS 190
#define PRINT_BUFFER_SIZE  (4096)

//#define BASE64_BUFFER_SIZE 3000

#include <stdarg.h>

#include "types.h"
#include "inlines.h"
#include "utils.h"

extern "C" uint16_t cdecl send_char_to_printer( char far *character );

class Screen {

  public:

    int8_t init( uint8_t backScrollPages, uint8_t initWrapMode, bool useMcga );


    // Primitives for writing on the physical screen.

    void clearConsole( void );

    void myCprintf( uint8_t x, uint8_t y, uint8_t attr, char *fmt, ... );
    void myCprintf_internal( uint8_t x, uint8_t y, uint8_t attr, char *fmt, va_list ap );

    inline void putch( uint8_t x, uint8_t y, uint8_t attr, char ch ) {
      uint16_t offset = ((y * terminalCols) + x) * 2;
      uint16_t tmp = (attr<<8 | ch );
      if ( preventSnow ) {
        writeCharWithoutSnow( screenBaseSeg, offset, tmp );
      } else {
        uint16_t far *start = (uint16_t far *)(Screen_base + offset);
        *start = tmp;
      }
    }

    inline void repeatCh( uint8_t x, uint8_t y, uint8_t attr, char ch, uint8_t count ) {
      uint16_t far *start = (uint16_t far *)(Screen_base + (y*terminalCols+x)*2);
      if ( preventSnow ) { waitForCGARetraceLong( ); }
      fillUsingWord( start, (attr<<8) | ch, count );
    }


    // Compute the address of the physical screen location for a given x and y

    inline void updateVidBufPtr( void ) {
      vidBufPtr = Screen_base + (cursor_x<<1) + (cursor_y*bytesPerLine);
    }


    // Primitives for handling the virtual screen, with possible side
    // effects on the physical screen.

    void scroll( void );
    void scrollInternal( void );

    void readWord( uint16_t x, uint16_t y );
    void setSelectionStart( uint16_t x, uint16_t y );
    void extendSelection( uint16_t x, uint16_t y );
    void extendSelectionRelative( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2 );
    void readLine( uint16_t y );
    void initializeSelection();
    void populateSelection();
    void highlightSelection( bool highlight );
    void updateSelectionPart( uint16_t start, uint16_t end );
    void updateHighlight();

    void add( char *buf );
    void add( char *buf, uint16_t len );

    void paint( void );
    void paint( int16_t offsetLines );

    void clear( uint16_t top_x, uint16_t top_y, uint16_t bot_x, uint16_t bot_y );

    void insLine( uint16_t line_y );
    void delLine( uint16_t line_y );

    void delChars( uint16_t len );
    void insChars( uint16_t len );
    void eraseChars( uint16_t len );


    // Compute an offset into the virtual buffer for a given x and y

    inline uint16_t ScrOffset( uint16_t x, uint16_t y ) {

      uint32_t tmp = topOffset;
      tmp = tmp + y*bytesPerLine+(x<<1);
      if ( tmp >= bufferSize ) tmp = tmp - bufferSize;

      return tmp;
    }

    uint16_t Screen::ScrRelative( uint16_t point ){
      if ( point >= topOffset ){
        return (point - topOffset);
      }
      else{
        return (bufferSize - topOffset) + point;
      }
    }

    bool isColorCard( void ) { return colorCard; }
    bool isPreventSnowOn( void ) { return preventSnow; }

    inline uint16_t getScreenBaseSeg( void ) { return screenBaseSeg; }

    void doNotUpdateRealScreen( void ) { updateRealScreen = false; }
    bool isVirtualScreenUpdated( void ) { return virtualUpdated; }

    void toggleWrapMode( void ) { wrapMode = !wrapMode; }
    bool isWrapModeOn( void ) { return wrapMode; }


    // The next set of methods are used by ANSI emulation code.

    void setHorizontal( int16_t newHorizontal );
    void setVertical( int16_t newVertical );
    void adjustVertical( int16_t delta );

    void suppressOverhang( void ) {

      // If we are waiting to wrap-around and an ESC sequence comes in
      // then complete the wrap-around.  I can't find documentation that
      // this is how it should work so this is reverse engineered.

      if ( overhang ) {
        cursor_x = 0;
        scroll( );
      }
      overhang = false;
    }

    void saveCursor( void );
    void restoreCursor( void );

    void resetTerminalState( bool useMcga );



    // Public class variables ...

    // Screen_base points to the start of the real frame buffer.  It is also
    // used as an indicator that this class has been initialized.  If for some
    // reason the class not been initialized correctly, it should be set to
    // NULL.

    uint8_t far *Screen_base;

    uint16_t terminalLines;     // How many lines for the terminal window?
    uint16_t terminalCols;      // How many columns in our terminal window?


    // Terminal emulation vars

    int16_t cursor_x;           // Cursor horizontal position
    int16_t cursor_y;           // Cursor vertical position
    int16_t scrollRegion_top;   // Top line of the scroll region
    int16_t scrollRegion_bottom;// Bottom line of the scroll region

    uint8_t curAttr;            // Current screen attribute
    uint8_t lastChar;           // Last printable char (used by some ANSI functions)

    bool originMode;
    bool autoWrap;
    bool usingGraphics;
    bool preventSnow;
    bool colorCard;         // Monochrome=false, CGA, EGA, or VGA=true
    bool padA;
    bool padB;
    bool padC;

    struct Selection {
      char far *buf;
      uint16_t  start;
      uint16_t  end;
      uint16_t  oldStart;
      uint16_t  oldEnd;
      bool      highlighted;
      bool      pad1;
      bool      pad2;
      bool      pad3;
      bool      pad4;
      bool      pad5;
      bool      pad6;
      bool      pad7;
    } selection;

    utfTrans far *transTable;
    int      transTableCount;
    int      transTableMidPoint;
    uint8_t  nonPrintable;

    uint8_t PrintingAllowed;
    uint8_t AutoPrint;
    uint8_t writeCount;
    char *printerBuffer;
    uint16_t printerReadIndex;
    uint16_t printerWriteIndex;
  //FILE *printerFile;

  //void send_char_to_file( char character );

  private:

    // Video card characteristics

    uint16_t screenBaseSeg;     // Segment for the screen framebuffer
    uint16_t bytesPerLine;      // How many bytes does a terminal line require?


    // Virtual buffer characteristics

    uint8_t far *buffer;        // Start of virtual screen buffer
    uint8_t far *bufferEnd;     // End of virtual screen buffer
    uint16_t     screenSize;    // Size of a single screen in bytes
    uint16_t     bufferSize;    // Size of buffer area in bytes
    uint16_t     topOffset;     // Offset in bytes to start of virtual screen
    uint16_t     totalLines;    // How many lines for viewable and backscroll?
    uint16_t backScrollOffset;  // If backscrolling is active, how far back?


    // Pointer to our current position on the real screen.
    // This is sometimes out of date but it gets refreshed.

    uint8_t far *vidBufPtr;     // Pointer into the real video buffer


    // Used by the user interface to suppress screen updating or to know when
    // a repaint is needed.

    bool    updateRealScreen;   // Should we be updating the live screen?
    bool    virtualUpdated;     // Have we updated the virtual screen?


    // Toggles

    uint8_t wrapMode;           // Are we wrapping around lines?


    // Overhang is complex.  When a terminal prints in it's last column it
    // does not immediately wrap to column 0.  This might be to allow for
    // a backspace without having to wrap backwards or unscroll the screen.
    // So we keep track of when we have printed a character in the last column
    // but have not wrapped yet.
    bool    overhang;

    struct CursorSaveArea {
      int16_t cursor_x;
      int16_t cursor_y;
      uint8_t curAttr;
      bool    originMode;
      bool    autoWrap;
      uint8_t padding;
    } cursorSaveArea;

};




#endif
