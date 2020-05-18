
/*

   mTCP Mouse.h
   Copyright (C) 2020 Jonathan H. Pyle (jhpyle@gmail.com)
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


   Description: Mouse interface for 8086

   Changes:
   2020-05-17: Initial release


*/


#ifndef _MOUSE_H
#define _MOUSE_H

#define MOUSE_LEFT 1
#define MOUSE_RIGHT 2
#define MOUSE_MIDDLE 4
//#define MOUSE_DOUBLE_START 8
//#define MOUSE_TICKS_DOUBLE_CLICK 5
#define MOUSE_TICKS_MIDDLE_CLICK 4
#define MOUSE_TICKS_SHOW_DURATION 140
#define MOUSE_UP 0
#define MOUSE_DOWN 1
#define XTERM_BUTTON_UP 0
#define XTERM_BUTTON_DOWN 1
#define XTERM_BUTTON_LEFT 0
#define XTERM_BUTTON_MIDDLE 1
#define XTERM_BUTTON_RIGHT 2

extern "C" uint16_t cdecl ibm_mouse_init( void );
extern "C" uint16_t cdecl ibm_mouse_show( void );
extern "C" uint16_t cdecl ibm_mouse_hide( void );
extern "C" uint16_t cdecl ibm_mouse_get_status( uint16_t far *buttons, uint16_t far *column, uint16_t far *row );
extern "C" uint16_t cdecl ibm_mouse_get_time( uint32_t far *time );
extern "C" uint16_t cdecl ibm_mouse_install( void );
extern "C" uint16_t cdecl ibm_mouse_uninstall( void );

typedef _Packed struct {
  bool enable;
  bool installed;
  bool showing;
  bool should_show;
  bool xterm_normal;
  bool xterm_motion;
  bool xterm_sgr;
  bool xterm_any;
  bool bracketed_paste;
  bool ignore_next_right_up;
  bool next_up_is_middle_up;
  bool ignore_next_left_up;
  uint8_t pending;
  uint16_t buttons;
  uint16_t column;
  uint16_t row;
  uint32_t prev_down;
  uint16_t prev_buttons;
} Mouse_t;

#endif
