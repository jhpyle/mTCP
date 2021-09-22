
/*

   mTCP Keys.h
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
   2019-05-10: Extended keyboard support

*/


#ifndef _KEYS_H
#define _KEYS_H

#include "types.h"


// Define human readable symbols for our keys

#define K_NoKey              (0)
#define K_NormalKey          (1)
#define K_CursorUp           (2)
#define K_CursorDown         (3)
#define K_CursorLeft         (4)
#define K_CursorRight        (5)
#define K_PageUp             (6)
#define K_PageDown           (7)
#define K_Home               (8)
#define K_Insert             (9)
#define K_Delete            (10)
#define K_Backtab           (11)
#define K_Alt_R             (12)
#define K_Alt_W             (13)
#define K_Alt_H             (14)
#define K_Alt_X             (15)
#define K_Alt_B             (16)
#define K_Enter             (17)
#define K_Alt_E             (18)
#define K_Alt_N             (19)
#define K_Alt_D             (20)
#define K_Alt_U             (21)
#define K_Alt_F             (22)
#define K_AltKey            (23)
#define K_Alt_CursorDown    (24)
#define K_Alt_CursorLeft    (25)
#define K_Alt_CursorRight   (26)
#define K_Alt_CursorUp      (27)
#define K_Alt_Delete        (28)
#define K_Alt_End           (29)
#define K_Alt_F1            (30)
#define K_Alt_F2            (32)
#define K_Alt_F3            (33)
#define K_Alt_F4            (34)
#define K_Alt_F5            (35)
#define K_Alt_F6            (36)
#define K_Alt_F7            (37)
#define K_Alt_F8            (38)
#define K_Alt_F9            (39)
#define K_Alt_F10           (40)
#define K_Alt_F11           (41)
#define K_Alt_F12           (42)
#define K_Alt_Home          (43)
#define K_Alt_Insert        (44)
#define K_Ctrl_Backspace    (45)
#define K_Ctrl_CursorDown   (46)
#define K_Ctrl_CursorLeft   (47)
#define K_Ctrl_CursorRight  (48)
#define K_Ctrl_CursorUp     (49)
#define K_Ctrl_Delete       (50)
#define K_Ctrl_End          (51)
#define K_Ctrl_Home         (52)
#define K_Ctrl_Insert       (53)
#define K_Ctrl_PageDown     (54)
#define K_Ctrl_PageUp       (55)
#define K_Ctrl_Tab          (56)
#define K_End               (57)
#define K_F1                (58)
#define K_F2                (59)
#define K_F3                (60)
#define K_F4                (61)
#define K_F5                (62)
#define K_F6                (63)
#define K_F7                (64)
#define K_F8                (65)
#define K_F9                (66)
#define K_F10               (67)
#define K_F11               (68)
#define K_F12               (69)
#define K_Ctrl_F1           (70)
#define K_Ctrl_F2           (72)
#define K_Ctrl_F3           (73)
#define K_Ctrl_F4           (74)
#define K_Ctrl_F5           (75)
#define K_Ctrl_F6           (76)
#define K_Ctrl_F7           (77)
#define K_Ctrl_F8           (78)
#define K_Ctrl_F9           (79)
#define K_Ctrl_F10          (80)
#define K_Ctrl_F11          (81)
#define K_Ctrl_F12          (82)
#define K_Shift_F1          (83)
#define K_Shift_F2          (84)
#define K_Shift_F3          (85)
#define K_Shift_F4          (86)
#define K_Shift_F5          (87)
#define K_Shift_F6          (88)
#define K_Shift_F7          (89)
#define K_Shift_F8          (90)
#define K_Shift_F9          (91)
#define K_Shift_F10         (92)
#define K_Shift_F11         (93)
#define K_Shift_F12         (94)
#define K_Local_PageDown    (95)
#define K_Local_PageUp      (96)
#define K_CenterKey         (97)
#define K_Ctrl_PrtScr       (98)

typedef struct {
  uint8_t specialKey;  // K_NoKey, K_NormalKey, or a special key code
  uint8_t local;       // Interpreted locally by telnet?
  uint8_t normalKey;   // Valid only if specialKey = K_NormalKey
} Key_t;

Key_t getKey( void );



#endif
