
/*

   mTCP Keys.cpp
   Copyright (C) 2011-2020 Michael B. Brutman (mbbrutman@gmail.com)
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


   Description: Keyboard handling routines shared by telnet and ymodem.

   Changes:

   2012-04-29: Split out from telnet when xmodem and ymodem were added
   2020-05-10: Extended keyboard support

*/


#include <bios.h>

#include "keys.h"


Key_t getKey( void ) {

  Key_t rc;
  rc.specialKey = K_NoKey;
  rc.local = 0;
  rc.normalKey = 0;

  uint16_t c = bioskey(K_KEY_GET);

  // Special key?
  if ( (c & 0xff) == 0 ) {

    uint8_t fkey = c>>8;

    switch ( fkey ) {
      case 0x0e: { rc.specialKey = K_AltKey;   rc.normalKey = 0x08;  break; }
      case 0x0f: { rc.specialKey = K_Backtab;                        break; }
      case 0x10: { rc.specialKey = K_AltKey;   rc.normalKey = 0x71;  break; }
      case 0x11: { rc.specialKey = K_Alt_W;           rc.local = 1;  break; }
      case 0x12: { rc.specialKey = K_Alt_E;           rc.local = 1;  break; }
      case 0x13: { rc.specialKey = K_Alt_R;           rc.local = 1;  break; }
      case 0x14: { rc.specialKey = K_AltKey;   rc.normalKey = 0x74;  break; }
      case 0x15: { rc.specialKey = K_AltKey;   rc.normalKey = 0x79;  break; }
      case 0x16: { rc.specialKey = K_Alt_U;           rc.local = 1;  break; }
      case 0x17: { rc.specialKey = K_AltKey;   rc.normalKey = 0x69;  break; }
      case 0x18: { rc.specialKey = K_AltKey;   rc.normalKey = 0x6f;  break; }
      case 0x19: { rc.specialKey = K_AltKey;   rc.normalKey = 0x70;  break; }
      case 0x1c: { rc.specialKey = K_AltKey;   rc.normalKey = 0x0d;  break; }
      case 0x1e: { rc.specialKey = K_AltKey;   rc.normalKey = 0x61;  break; }
      case 0x1f: { rc.specialKey = K_AltKey;   rc.normalKey = 0x73;  break; }
      case 0x20: { rc.specialKey = K_Alt_D;           rc.local = 1;  break; }
      case 0x21: { rc.specialKey = K_Alt_F;           rc.local = 1;  break; }
      case 0x22: { rc.specialKey = K_AltKey;   rc.normalKey = 0x67;  break; }
      case 0x23: { rc.specialKey = K_Alt_H;           rc.local = 1;  break; }
      case 0x24: { rc.specialKey = K_AltKey;   rc.normalKey = 0x6a;  break; }
      case 0x25: { rc.specialKey = K_AltKey;   rc.normalKey = 0x6b;  break; }
      case 0x26: { rc.specialKey = K_AltKey;   rc.normalKey = 0x6c;  break; }
      case 0x27: { rc.specialKey = K_AltKey;   rc.normalKey = 0x3b;  break; }
      case 0x28: { rc.specialKey = K_AltKey;   rc.normalKey = 0x27;  break; }
      case 0x29: { rc.specialKey = K_AltKey;   rc.normalKey = 0x60;  break; }
      case 0x2b: { rc.specialKey = K_AltKey;   rc.normalKey = 0x5c;  break; }
      case 0x2c: { rc.specialKey = K_AltKey;   rc.normalKey = 0x7a;  break; }
      case 0x2d: { rc.specialKey = K_Alt_X;           rc.local = 1;  break; }
      case 0x2e: { rc.specialKey = K_AltKey;   rc.normalKey = 0x63;  break; }
      case 0x2f: { rc.specialKey = K_AltKey;   rc.normalKey = 0x76;  break; }
      case 0x31: { rc.specialKey = K_Alt_N;           rc.local = 1;  break; }
      case 0x32: { rc.specialKey = K_AltKey;   rc.normalKey = 0x6d;  break; }
      case 0x33: { rc.specialKey = K_AltKey;   rc.normalKey = 0x2c;  break; }
      case 0x34: { rc.specialKey = K_AltKey;   rc.normalKey = 0x2e;  break; }
      case 0x35: { rc.specialKey = K_AltKey;   rc.normalKey = 0x2f;  break; }
      case 0x39: { rc.specialKey = K_Alt_B;           rc.local = 1;  break; }
      case 0x3B: { rc.specialKey = K_F1;                             break; }
      case 0x3C: { rc.specialKey = K_F2;                             break; }
      case 0x3D: { rc.specialKey = K_F3;                             break; }
      case 0x3E: { rc.specialKey = K_F4;                             break; }
      case 0x3F: { rc.specialKey = K_F5;                             break; }
      case 0x40: { rc.specialKey = K_F6;                             break; }
      case 0x41: { rc.specialKey = K_F7;                             break; }
      case 0x42: { rc.specialKey = K_F8;                             break; }
      case 0x43: { rc.specialKey = K_F9;                             break; }
      case 0x44: { rc.specialKey = K_F10;                            break; }
      case 0x47: { rc.specialKey = K_Home;                           break; }
      case 0x48: { rc.specialKey = K_CursorUp;                       break; }
      case 0x49: { rc.specialKey = K_PageUp;                         break; }
      case 0x4a: { rc.specialKey = K_AltKey; rc.normalKey = 0x2d;    break; }
      case 0x4b: { rc.specialKey = K_CursorLeft;                     break; }
      case 0x4c: { rc.specialKey = K_CenterKey;                      break; }
      case 0x4d: { rc.specialKey = K_CursorRight;                    break; }
      case 0x4e: { rc.specialKey = K_AltKey; rc.normalKey = 0x2b;    break; }
      case 0x4f: { rc.specialKey = K_End;                            break; }
      case 0x50: { rc.specialKey = K_CursorDown;                     break; }
      case 0x51: { rc.specialKey = K_PageDown;                       break; }
      case 0x52: { rc.specialKey = K_Insert;                         break; }
      case 0x53: { rc.specialKey = K_Delete;                         break; }
      case 0x54: { rc.specialKey = K_Shift_F1;                       break; }
      case 0x55: { rc.specialKey = K_Shift_F2;                       break; }
      case 0x56: { rc.specialKey = K_Shift_F3;                       break; }
      case 0x57: { rc.specialKey = K_Shift_F4;                       break; }
      case 0x58: { rc.specialKey = K_Shift_F5;                       break; }
      case 0x59: { rc.specialKey = K_Shift_F6;                       break; }
      case 0x5a: { rc.specialKey = K_Shift_F7;                       break; }
      case 0x5b: { rc.specialKey = K_Shift_F8;                       break; }
      case 0x5c: { rc.specialKey = K_Shift_F9;                       break; }
      case 0x5d: { rc.specialKey = K_Shift_F10;                      break; }
      case 0x5e: { rc.specialKey = K_Ctrl_F1;                        break; }
      case 0x5f: { rc.specialKey = K_Ctrl_F2;                        break; }
      case 0x60: { rc.specialKey = K_Ctrl_F3;                        break; }
      case 0x61: { rc.specialKey = K_Ctrl_F4;                        break; }
      case 0x62: { rc.specialKey = K_Ctrl_F5;                        break; }
      case 0x63: { rc.specialKey = K_Ctrl_F6;                        break; }
      case 0x64: { rc.specialKey = K_Ctrl_F7;                        break; }
      case 0x65: { rc.specialKey = K_Ctrl_F8;                        break; }
      case 0x66: { rc.specialKey = K_Ctrl_F9;                        break; }
      case 0x67: { rc.specialKey = K_Ctrl_F10;                       break; }
      case 0x68: { rc.specialKey = K_Alt_F1;                         break; }
      case 0x69: { rc.specialKey = K_Alt_F2;                         break; }
      case 0x6A: { rc.specialKey = K_Alt_F3;                         break; }
      case 0x6B: { rc.specialKey = K_Alt_F4;                         break; }
      case 0x6C: { rc.specialKey = K_Alt_F5;                         break; }
      case 0x6D: { rc.specialKey = K_Alt_F6;                         break; }
      case 0x6E: { rc.specialKey = K_Alt_F7;                         break; }
      case 0x6F: { rc.specialKey = K_Alt_F8;                         break; }
      case 0x70: { rc.specialKey = K_Alt_F9;                         break; }
      case 0x71: { rc.specialKey = K_Alt_F10;                        break; }
      case 0x72: { rc.specialKey = K_Ctrl_PrtScr;                    break; }
      case 0x73: { rc.specialKey = K_Ctrl_CursorLeft;                break; }
      case 0x74: { rc.specialKey = K_Ctrl_CursorRight;               break; }
      case 0x75: { rc.specialKey = K_Ctrl_End;                       break; }
      case 0x76: { rc.specialKey = K_Local_PageDown;  rc.local = 1;  break; }
      case 0x77: { rc.specialKey = K_Ctrl_Home;                      break; }
      case 0x78: { rc.specialKey = K_AltKey;   rc.normalKey = 0x31;  break; }
      case 0x79: { rc.specialKey = K_AltKey;   rc.normalKey = 0x32;  break; }
      case 0x7a: { rc.specialKey = K_AltKey;   rc.normalKey = 0x33;  break; }
      case 0x7b: { rc.specialKey = K_AltKey;   rc.normalKey = 0x34;  break; }
      case 0x7c: { rc.specialKey = K_AltKey;   rc.normalKey = 0x35;  break; }
      case 0x7d: { rc.specialKey = K_AltKey;   rc.normalKey = 0x36;  break; }
      case 0x7e: { rc.specialKey = K_AltKey;   rc.normalKey = 0x37;  break; }
      case 0x7f: { rc.specialKey = K_AltKey;   rc.normalKey = 0x38;  break; }
      case 0x80: { rc.specialKey = K_AltKey;   rc.normalKey = 0x39;  break; }
      case 0x81: { rc.specialKey = K_AltKey;   rc.normalKey = 0x30;  break; }
      case 0x82: { rc.specialKey = K_AltKey;   rc.normalKey = 0x2d;  break; }
      case 0x83: { rc.specialKey = K_AltKey;   rc.normalKey = 0x3d;  break; }
      case 0x84: { rc.specialKey = K_Local_PageUp;    rc.local = 1;  break; }
      case 0x85: { rc.specialKey = K_F11;                            break; }
      case 0x86: { rc.specialKey = K_F12;                            break; }
      case 0x8b: { rc.specialKey = K_Alt_F11;                        break; }
      case 0x8c: { rc.specialKey = K_Alt_F12;                        break; }
      case 0x8d: { rc.specialKey = K_Ctrl_CursorUp;                  break; }
      case 0x8e: { rc.specialKey = K_NormalKey; rc.normalKey = 0x1f; break; }
      case 0x91: { rc.specialKey = K_Ctrl_CursorDown;                break; }
      case 0x92: { rc.specialKey = K_Ctrl_Insert;                    break; }
      case 0x93: { rc.specialKey = K_Ctrl_Delete;                    break; }
      case 0x94: { rc.specialKey = K_Ctrl_Tab;                       break; }
      case 0x97: { rc.specialKey = K_Alt_Home;                       break; }
      case 0x98: { rc.specialKey = K_Alt_CursorUp;                   break; }
      case 0x99: { rc.specialKey = K_Local_PageUp;    rc.local = 1;  break; }
      case 0x9b: { rc.specialKey = K_Alt_CursorLeft;                 break; }
      case 0x9d: { rc.specialKey = K_Alt_CursorRight;                break; }
      case 0x9f: { rc.specialKey = K_Alt_End;                        break; }
      case 0xa0: { rc.specialKey = K_Alt_CursorDown;                 break; }
      case 0xa1: { rc.specialKey = K_Local_PageDown;  rc.local = 1;  break; }
      case 0xa2: { rc.specialKey = K_Alt_Insert;                     break; }
      case 0xa3: { rc.specialKey = K_Alt_Delete;                     break; }
      case 0xa4: { rc.specialKey = K_AltKey;   rc.normalKey = 0x5c;  break; }
      case 0xa5: { rc.specialKey = K_AltKey;   rc.normalKey = 0x09;  break; }
      case 0xa6: { rc.specialKey = K_AltKey;   rc.normalKey = 0x0d;  break; }
    }
  }
  else if ( (c & 0xff) == 0xE0 ) {

    uint8_t fkey = c>>8;

    switch ( fkey ) {
      case 0x47: { rc.specialKey = K_Home;                           break; }
      case 0x48: { rc.specialKey = K_CursorUp;                       break; }
      case 0x49: { rc.specialKey = K_PageUp;                         break; }
      case 0x4b: { rc.specialKey = K_CursorLeft;                     break; }
      case 0x4d: { rc.specialKey = K_CursorRight;                    break; }
      case 0x4f: { rc.specialKey = K_End;                            break; }
      case 0x50: { rc.specialKey = K_CursorDown;                     break; }
      case 0x51: { rc.specialKey = K_PageDown;                       break; }
      case 0x52: { rc.specialKey = K_Insert;                         break; }
      case 0x53: { rc.specialKey = K_Delete;                         break; }
      case 0x73: { rc.specialKey = K_Ctrl_CursorLeft;                break; }
      case 0x74: { rc.specialKey = K_Ctrl_CursorRight;               break; }
      case 0x75: { rc.specialKey = K_Ctrl_End;                       break; }
      case 0x76: { rc.specialKey = K_Ctrl_PageDown;                  break; }
      case 0x77: { rc.specialKey = K_Ctrl_Home;                      break; }
      case 0x84: { rc.specialKey = K_Ctrl_PageUp;                    break; }
      case 0x8d: { rc.specialKey = K_Ctrl_CursorUp;                  break; }
      case 0x91: { rc.specialKey = K_Ctrl_CursorDown;                break; }
      case 0x92: { rc.specialKey = K_Ctrl_Insert;                    break; }
      case 0x93: { rc.specialKey = K_Ctrl_Delete;                    break; }
    }
  }
  else {

    rc.specialKey = K_NormalKey;
    rc.normalKey = ( c & 0xff );

    // Special enter key processing - we want to be able to tell the
    // difference between Enter and Ctrl-M.
    if ( rc.normalKey == 13 ) {
      if ( (c>>8) == 0x1c ) { // This was the Enter key.
        rc.specialKey = K_Enter;
        rc.normalKey = 0;
      }
    }
    else if ( rc.normalKey == 0x7f ) {
      rc.specialKey = K_Ctrl_Backspace;
      rc.normalKey = 0;
    }
  }

  return rc;
}

