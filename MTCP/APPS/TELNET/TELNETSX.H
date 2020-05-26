/*

   mTCP TelnetSx.h
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


   Description: Sixel graphics

   Changes:
   2020-05-25: Initial release

*/


#ifndef _SIXEL_H
#define _SIXEL_H

#define CyanMagenta       (0)
#define LightCyanMagenta  (1)
#define GreenRed          (2)
#define LightGreenRed     (3)
#define MCGA256           (4)
#define Mono              (5)

#include <stdarg.h>
#include <bios.h>

#include "types.h"
#include "inlines.h"

#define DCS_ARGS (16)

extern "C" uint16_t cdecl start_graphics( uint8_t far *mode );
extern "C" uint16_t cdecl set_cga_palette( uint8_t far *pal_type, uint8_t far *background );
extern "C" uint16_t cdecl set_mcga_palette( uint8_t far *palette );
extern "C" uint16_t cdecl write_mcga_graphics( uint8_t far *image );
extern "C" uint16_t cdecl write_cga_graphics( uint8_t far *image );
extern "C" uint16_t cdecl stop_graphics( void );

class Graphics {

public:

  uint16_t processDCSSeq( uint8_t *buffer, uint16_t len );
  void     reset( void );
  void     showImage( void );
  void     returnToTextMode( void );
  void     initializeGraphics( void );
  int16_t  sixelPh;
  int16_t  sixelPv;
  int8_t   sixelInvalid;
  uint8_t  sixelColors;
  bool     sixelReady;
  bool     done;
  bool     enabled;
  bool     useMcga;

private:

  bool     DCSParamSeen;
  bool     graphicsMode;
  bool     initStage;
  bool     pad1;

  //void     set_hls(int16_t color_index, int16_t hue, int16_t lum, int16_t sat);
  void     set_rgb(int16_t color_index, int16_t r, int16_t g, int16_t b);

  enum SixelParseState_t { LookForDCSParams, Sixel, SixelIgnore, SawEscape, DECGRA, DECGRI, DECGCI };
  SixelParseState_t sixelParseState;

  int16_t  DCSParamValue;
  int16_t  sixelPad;
  int16_t  sixelPan;
  int16_t  sixelX;
  int16_t  sixelY;
  int16_t  sixel_n;
  int16_t  sixel_i;
  int16_t  sixel_bits;
  int16_t  sixel_vertical_mask;
  int16_t  sixelStartX;
  int16_t  sixel_c;
  int16_t  sixel_repeat_count;
  int16_t  sixel_current_color;
  uint16_t hmax;
  uint16_t vmax;
  int16_t  parms[ DCS_ARGS ];  // Array of parameters
  uint16_t parmsFound;         // Number of parameters found
  uint8_t  oldVideoMode;
  uint8_t  videoMode;
  uint16_t bufferLength;

  uint8_t *palette;
  uint8_t *image;

};

#endif
