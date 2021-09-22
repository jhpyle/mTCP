/*

   mTCP TelnetSx.cpp
   Copyright (C) 2009-2020 Michael B. Brutman (mbbrutman@gmail.com)
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


   Description: Sixel graphics support

   Changes:
   2020-05-25: Initial release
   2020-06-07: Fixed bugs and made more efficient.
   2021-09-21: Added Hercules Graphics Card support.

*/


#include <stdlib.h>
#include <string.h>


#include "types.h"
#include "telnetsx.h"

void Graphics::reset() {
  if ( enabled ){
    sixelParseState = LookForDCSParams;
    DCSParamValue = 0;
    DCSParamSeen = false;
    parmsFound = 0;
    sixelInvalid = 0;
    sixelColors = 0;
    sixelPad = 1;
    sixelPan = 2;
    sixelPh = 0;
    sixelPv = 0;
    sixelX = 0;
    sixelY = 0;
    initStage = true;
    sixelStartX = 0;
    if ( !useHercules ){
      if ( useMcga ){
        memset(palette, 0, 256*3);
      }
      else {
        memset(palette, 0, 4*3);
      }
    }
  }
  else{
    sixelParseState = SixelIgnore;
  }
  done = false;
}

uint16_t Graphics::processDCSSeq( uint8_t *buffer, uint16_t len ) {
  uint16_t i = 0;
  while ( i < len ) {
    uint8_t c = buffer[i];
    if ( c == 0x1b ) {
      sixelParseState = SawEscape;
    }
    else if ( sixelParseState == SixelIgnore ) {
      // ignore the byte
    }
    else if ( sixelParseState == SawEscape ) {
      done = true;
      if ( c == 0x5c && enabled ) {
        sixelReady = true;
      }
      return ++i;
    }
    else if ( sixelParseState == LookForDCSParams ) {
      switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        DCSParamSeen = true;
        DCSParamValue = DCSParamValue * 10 + c - '0';
        break;
      case ';':
        parms[parmsFound] = DCSParamValue;
        if (parmsFound < (DCS_ARGS - 1)) {
          parmsFound++;
        }
        DCSParamValue = 0;
        break;
      case 'q':
        if (DCSParamSeen) {
          parms[parmsFound] = DCSParamValue;
          if (parmsFound < (DCS_ARGS - 1)) {
            parmsFound++;
          }
          DCSParamValue = 0;
        }
        if (parmsFound > 0) {
          switch (parms[0]) {
          case 0:
          case 1:
            sixelPad = 2;
            break;
          case 2:
            sixelPad = 5;
            break;
          case 3:
          case 4:
            sixelPad = 4;
            break;
          case 5:
          case 6:
            sixelPad = 3;
            break;
          case 7:
          case 8:
            sixelPad = 2;
            break;
          case 9:
            sixelPad = 1;
            break;
          default:
            sixelPad = 2;
            break;
          }
        }
        if (parmsFound > 2) {
          if (parms[2] == 0) {
            parms[2] = 10;
          }
          sixelPan = sixelPan * parms[2] / 10;
          sixelPad = sixelPad * parms[2] / 10;
          if (sixelPan <= 0) {
            sixelPan = 1;
          }
          if (sixelPad <= 0) {
            sixelPad = 1;
          }
        }
        parmsFound = 0;
        sixelParseState = Sixel;
        break;
      default:
        break;
      }
    }
    else if ( sixelParseState == Sixel ) {
      switch (c) {
      case '"':
        parmsFound = 0;
        DCSParamValue = 0;
        sixelParseState = DECGRA;
        break;
      case '!':
        parmsFound = 0;
        DCSParamValue = 0;
        sixelParseState = DECGRI;
        break;
      case '#':
        parmsFound = 0;
        DCSParamValue = 0;
        sixelParseState = DECGCI;
        break;
      case '$':
        sixelX = sixelStartX;
        break;
      case '-':
        sixelX = sixelStartX;
        sixelY += 6;
        if ( sixelY >= vmax ) {
           sixelY = vmax - 1;
        }
        break;
      default:
        if ( initStage ) {
          initStage = false;
          if ( sixelPh == 0 || sixelPv == 0 ) {
            sixelParseState = SixelIgnore;
            sixelInvalid = 2;
            break;
          }
          if (useHercules) {
            if (sixelColors != 0) {
              sixelParseState = SixelIgnore;
              sixelInvalid = 3;
              break;
            }
            videoMode = Mono;
            hmax = 720;
            vmax = 348;
            sixelFunc = &Graphics::sixel_Mono_Hercules;
            clear_hercules();
          }
          else {
            if (sixelColors == 0) {
              videoMode = Mono;
              hmax = 640;
              if ( useMcga ) {
                vmax = 480;
                sixelFunc = &Graphics::sixel_Mono_Mcga;
              }
              else {
                vmax = 200;
                sixelFunc = &Graphics::sixel_Mono_Cga;
              }
            }
            else {
              hmax = 320;
              vmax = 200;
              if ( sixelColors == 4 ) {
                if ( palette[6] == 62 && palette[7] == 19 && palette[8] == 62 ) {
                  videoMode = LightCyanMagenta;
                  sixelFunc = &Graphics::sixel_Cga;
                }
                else if ( palette[6] == 42 && palette[7] == 0 && palette[8] == 42 ) {
                  videoMode = CyanMagenta;
                  sixelFunc = &Graphics::sixel_Cga;
                }
                else if ( palette[6] == 62 && palette[7] == 19 && palette[8] == 19 ) {
                  videoMode = LightGreenRed;
                  sixelFunc = &Graphics::sixel_Cga;
                }
                else if ( palette[6] == 42 && palette[7] == 0 && palette[8] == 0 ) {
                  videoMode = GreenRed;
                  sixelFunc = &Graphics::sixel_Cga;
                }
                else if ( useMcga ){
                  videoMode = MCGA256;
                  sixelFunc = &Graphics::sixel_Mcga256;
                }
                else {
                  videoMode = LightCyanMagenta;
                  sixelFunc = &Graphics::sixel_Cga;
                }
              }
              else if ( useMcga ){
                videoMode = MCGA256;
                sixelFunc = &Graphics::sixel_Mcga256;
              }
              else {
                videoMode = LightCyanMagenta;
                sixelFunc = &Graphics::sixel_Cga;
              }
            }
            if ( videoMode == Mono && useMcga ) {
              memset(image, 0x00, 64000);
            }
            if ( videoMode == MCGA256 ){
              memset(image, 0xff, 64000);
            }
            else{
              memset(image, 0x00, 16384);
            }
          }
          if ( sixelPh > hmax || sixelPv > vmax ) {
            sixelParseState = SixelIgnore;
            sixelInvalid = 1;
            break;
          }
          else {
            sixelX = ( hmax - sixelPh ) >> 1;
            sixelY = ( vmax - sixelPv ) >> 1;
            sixelStartX = sixelX;
          }
        }
        if (c >= '?' && c <= '~') {
          sixel_bits = c - '?';
          if (sixel_bits == 0){
          // if (sixel_bits == 0 && !( useHercules && videoMode == Mono)) {
            sixelX += sixel_repeat_count;
            if ( sixelX >= hmax ) {
              sixelX = hmax - 1;
            }
          }
          else {
            (this->*sixelFunc)();
          }
          sixel_repeat_count = 1;
        }
        break;
      }
    }
    else if ( sixelParseState == DECGRA ) {
      switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        DCSParamValue = DCSParamValue * 10 + c - '0';
        break;
      case ';':
        parms[parmsFound] = DCSParamValue;
        if (parmsFound < ( DCS_ARGS - 1 )) {
          parmsFound++;
        }
        DCSParamValue = 0;
        break;
      default:
        parms[parmsFound] = DCSParamValue;
        if ( parmsFound < ( DCS_ARGS - 1 ) ) {
          parmsFound++;
        }
        DCSParamValue = 0;
        if ( parmsFound > 0 ) {
          sixelPad = parms[0];
        }
        if ( parmsFound > 1 ) {
          sixelPan = parms[1];
        }
        if ( parmsFound > 2 && parms[2] > 0 ) {
          sixelPh = parms[2] - 1;
        }
        if ( parmsFound > 3 && parms[3] > 0 ) {
          sixelPv = parms[3] - 1;
        }
        if ( sixelPad <= 0 ) {
          sixelPad = 1;
        }
        if ( sixelPan <= 0 ) {
          sixelPan = 1;
        }
        parmsFound = 0;
        sixelParseState = Sixel;
        continue;
      }
    }
    else if ( sixelParseState == DECGRI ) {
      switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        DCSParamValue = DCSParamValue * 10 + c - '0';
        break;
      default:
        sixel_repeat_count = DCSParamValue;
        if ( sixel_repeat_count == 0 ) {
          sixel_repeat_count = 1;
        }
        sixelParseState = Sixel;
        DCSParamValue = 0;
        parmsFound = 0;
        continue;
      }
    }
    else if ( sixelParseState == DECGCI ) {
      switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        DCSParamValue = DCSParamValue * 10 + c - '0';
        break;
      case ';':
        parms[parmsFound] = DCSParamValue;
        if ( parmsFound < ( DCS_ARGS - 1 ) ) {
          parmsFound++;
        }
        DCSParamValue = 0;
        break;
      default:
        parms[parmsFound] = DCSParamValue;
        if ( parmsFound < ( DCS_ARGS - 1 ) ) {
          parmsFound++;
        }
        DCSParamValue = 0;
        if ( parmsFound > 0 ) {
          sixel_current_color = parms[0];
        }
        if ( sixel_current_color < 0 ) {
          sixel_current_color = 0;
        }
        else if ( sixel_current_color > 255 ) {
          sixel_current_color = 255;
        }
        if ( parmsFound > 4 ) {
          if ( parms[1] == 1 ) {
            /* HLS */
            if ( parms[2] > 360 ) {
              parms[2] = 360;
            }
            if ( parms[3] > 100 ) {
              parms[3] = 100;
            }
            if ( parms[4] > 100 ) {
              parms[4] = 100;
            }
            //set_hls(sixel_current_color, parms[2], parms[3], parms[4]);
            set_rgb(sixel_current_color, parms[2], parms[3], parms[4]);
          }
          else if ( parms[1] == 2 ) {
            /* RGB */
            if ( parms[2] > 100 ) {
              parms[2] = 100;
            }
            if ( parms[3] > 100 ) {
              parms[3] = 100;
            }
            if ( parms[4] > 100 ) {
              parms[4] = 100;
            }
            sixelColors++;
            set_rgb(sixel_current_color, parms[2], parms[3], parms[4]);
          }
        }
        sixelParseState = Sixel;
        parmsFound = 0;
        continue;
      }
    }
    i++;
  }
  return i;
}

void Graphics::sixel_Mono_Mcga() {
  sixel_vertical_mask = 0x01;
  if (sixel_repeat_count <= 1) {
    for (sixel_i = 0; sixel_i < 6; sixel_i++) {
      if ((sixel_bits & sixel_vertical_mask) != 0) {
        image[(sixelY + sixel_i) * 80 + (sixelX >> 3)] |= (1 << (7 - ( sixelX & 7 )));
      }
      sixel_vertical_mask <<= 1;
    }
    sixelX += 1;
    if ( sixelX >= hmax ) {
      sixelX = hmax - 1;
    }
  }
  else {
    if ( sixelX + sixel_repeat_count >= hmax ) {
      sixel_repeat_count = hmax - ( 1 + sixelX );
    }
    for (sixel_i = 0; sixel_i < 6; sixel_i++) {
      if ((sixel_bits & sixel_vertical_mask) != 0) {
        sixel_c = sixel_vertical_mask << 1;
        for (sixel_n = 1; (sixel_i + sixel_n) < 6; sixel_n++) {
          if ((sixel_bits & sixel_c) == 0) {
            break;
          }
          sixel_c <<= 1;
        }
        for (int16_t y = sixelY + sixel_i; y < (sixelY + sixel_i + sixel_n); ++y) {
          for (int16_t r = 0; r < sixel_repeat_count; ++r) {
            image[y * 80 + ((sixelX + r) >> 3)] |= (1 << (7 - ( (sixelX + r) & 7 )));
          }
        }
        sixel_i += (sixel_n - 1);
        sixel_vertical_mask <<= (sixel_n - 1);
      }
      sixel_vertical_mask <<= 1;
    }
    sixelX += sixel_repeat_count;
  }
}

void Graphics::sixel_Mono_Cga() {
  sixel_vertical_mask = 0x01;
  if (sixel_repeat_count <= 1) {
    for (sixel_i = 0; sixel_i < 6; sixel_i++) {
      if ((sixel_bits & sixel_vertical_mask) != 0) {
        image[( ( ( sixelY + sixel_i) & 1 ) ? 0x2000 : 0 ) + ( ( sixelY + sixel_i ) >> 1) * 80 + (sixelX >> 3)] |= (1 << (7 - ( sixelX & 7 )));
      }
      sixel_vertical_mask <<= 1;
    }
    sixelX += 1;
    if ( sixelX >= hmax ) {
      sixelX = hmax - 1;
    }
  }
  else {
    if ( sixelX + sixel_repeat_count >= hmax ) {
      sixel_repeat_count = hmax - ( 1 + sixelX );
    }
    for (sixel_i = 0; sixel_i < 6; sixel_i++) {
      if ((sixel_bits & sixel_vertical_mask) != 0) {
        sixel_c = sixel_vertical_mask << 1;
        for (sixel_n = 1; (sixel_i + sixel_n) < 6; sixel_n++) {
          if ((sixel_bits & sixel_c) == 0) {
            break;
          }
          sixel_c <<= 1;
        }
        for (int16_t y = sixelY + sixel_i; y < (sixelY + sixel_i + sixel_n); ++y) {
          for (int16_t r = 0; r < sixel_repeat_count; ++r) {
            image[( ( y & 1 ) ? 0x2000 : 0 ) + (y >> 1) * 80 + ((sixelX + r) >> 3)] |= (1 << (7 - ( (sixelX + r) & 7 )));
          }
        }
        sixel_i += (sixel_n - 1);
        sixel_vertical_mask <<= (sixel_n - 1);
      }
      sixel_vertical_mask <<= 1;
    }
    sixelX += sixel_repeat_count;
  }
}

void Graphics::sixel_Mono_Hercules() {
  sixel_vertical_mask = 0x01;
  if (sixel_repeat_count <= 1) {
    uint8_t sixel_horizontal_mask = (1 << (7 - ( sixelX % 8 )));
    for (sixel_i = 0; sixel_i < 6; sixel_i++) {
      if ((sixel_bits & sixel_vertical_mask) != 0) {
        image[0x2000 * ((sixelY + sixel_i) % 4) + 90 * ((sixelY + sixel_i) >> 2) + (sixelX >> 3)] |= sixel_horizontal_mask;
      }
      sixel_vertical_mask <<= 1;
    }
    sixelX += 1;
  }
  else {
    int lightPixel;
    if ( (sixelX + sixel_repeat_count) >= hmax ) {
      sixel_repeat_count = hmax - ( 1 + sixelX );
    }
    for (sixel_i = 0; sixel_i < 6; sixel_i++) {
      if ((sixel_bits & sixel_vertical_mask) != 0) {
        lightPixel = 1;
      }
      else{
        lightPixel = 0;
      }
      for (int16_t r = 0; r < sixel_repeat_count; r++) {
        if (lightPixel == 1){
          image[0x2000 * ((sixelY + sixel_i) % 4) + 90 * ((sixelY + sixel_i) >> 2) + (( sixelX + r ) >> 3)] |= (1 << (7 - ( (sixelX + r) % 8 )));
        }
      }
      sixel_vertical_mask <<= 1;
    }
    sixelX = sixelX + sixel_repeat_count;
  }
}

void Graphics::sixel_Mcga256() {
  sixel_vertical_mask = 0x01;
  if (sixel_repeat_count <= 1) {
    for (sixel_i = 0; sixel_i < 6; sixel_i++) {
      if ((sixel_bits & sixel_vertical_mask) != 0) {
        image[(sixelY + sixel_i) * 320 + sixelX] = sixel_current_color;
      }
      sixel_vertical_mask <<= 1;
    }
    sixelX += 1;
    if ( sixelX >= hmax ) {
      sixelX = hmax - 1;
    }
  }
  else {
    if ( sixelX + sixel_repeat_count >= hmax ) {
      sixel_repeat_count = hmax - ( 1 + sixelX );
    }
    for (sixel_i = 0; sixel_i < 6; sixel_i++) {
      if ((sixel_bits & sixel_vertical_mask) != 0) {
        sixel_c = sixel_vertical_mask << 1;
        for (sixel_n = 1; (sixel_i + sixel_n) < 6; sixel_n++) {
          if ((sixel_bits & sixel_c) == 0) {
            break;
          }
          sixel_c <<= 1;
        }
        for (int16_t y = sixelY + sixel_i; y < (sixelY + sixel_i + sixel_n); ++y) {
          memset(image + (size_t)y * 320 + (size_t)sixelX, sixel_current_color, (size_t)sixel_repeat_count);
        }
        sixel_i += (sixel_n - 1);
        sixel_vertical_mask <<= (sixel_n - 1);
      }
      sixel_vertical_mask <<= 1;
    }
    sixelX += sixel_repeat_count;
  }
}

void Graphics::sixel_Cga() {
  sixel_vertical_mask = 0x01;
  if (sixel_repeat_count <= 1) {
    for (sixel_i = 0; sixel_i < 6; sixel_i++) {
      if ((sixel_bits & sixel_vertical_mask) != 0) {
        image[( ( ( sixelY + sixel_i) & 1 ) ? 0x2000 : 0 ) + ( ( sixelY + sixel_i ) >> 1) * 80 + (sixelX >> 2)] |= ((sixel_current_color & 3) << (6 - ((sixelX & 3) << 1)));
      }
      sixel_vertical_mask <<= 1;
    }
    sixelX += 1;
    if ( sixelX >= hmax ) {
      sixelX = hmax - 1;
    }
  }
  else {
    if ( sixelX + sixel_repeat_count >= hmax ) {
      sixel_repeat_count = hmax - ( 1 + sixelX );
    }
    for (sixel_i = 0; sixel_i < 6; sixel_i++) {
      if ((sixel_bits & sixel_vertical_mask) != 0) {
        sixel_c = sixel_vertical_mask << 1;
        for (sixel_n = 1; (sixel_i + sixel_n) < 6; sixel_n++) {
          if ((sixel_bits & sixel_c) == 0) {
            break;
          }
          sixel_c <<= 1;
        }
        for (int16_t y = sixelY + sixel_i; y < (sixelY + sixel_i + sixel_n); ++y) {
          for (int16_t r = 0; r < sixel_repeat_count; ++r) {
            image[( ( y & 1 ) ? 0x2000 : 0 ) + (y >> 1) * 80 + ((sixelX + r) >> 2)] |= ((sixel_current_color & 3) << (6 - (((sixelX + r) & 3) << 1)));
          }
        }
        sixel_i += (sixel_n - 1);
        sixel_vertical_mask <<= (sixel_n - 1);
      }
      sixel_vertical_mask <<= 1;
    }
    sixelX += sixel_repeat_count;
  }
}

void Graphics::showImage() {
  uint8_t mode;
  if ( ( !graphicsMode ) || oldVideoMode != videoMode ) {
    if ( useHercules ){
      start_hercules();
    }
    else {
      if ( videoMode == Mono ) {
        if ( useMcga ){
          mode = 0x11;
        }
        else {
          mode = 0x06;
        }
      }
      else {
        if ( videoMode == MCGA256 ){
          mode = 0x13;
        }
        else {
          mode = 0x04;
        }
      }
      start_graphics(&mode);
    }
    oldVideoMode = videoMode;
    graphicsMode = true;
  }
  if ( useHercules ){
    return;
  }
  if ( !useMcga ){
    waitForCGARetraceLong( );
  }
  if ( videoMode == MCGA256 ) {
    set_mcga_palette(palette);
  }
  if ( videoMode == MCGA256 || ( videoMode == Mono && useMcga ) ) {
    write_mcga_graphics(image);
  }
  else {
    if ( videoMode != Mono ) {
      uint8_t pal_type;
      uint8_t background;
      if ( videoMode == LightCyanMagenta ){
        pal_type = 1;
        background = 16;
      }
      else if ( videoMode == CyanMagenta ){
        pal_type = 1;
        background = 0;
      }
      else if ( videoMode == LightGreenRed ){
        pal_type = 0;
        background = 16;
      }
      else {
        pal_type = 0;
        background = 0;
      }
      set_cga_palette(&pal_type, &background);
    }
    write_cga_graphics(image);
  }
}

void Graphics::initializeGraphics() {
  graphicsMode = false;
  oldVideoMode = 255;
  sixelReady = false;
  if ( useHercules ) {
    enabled = true;
    image = (uint8_t far *)MK_FP( SCREENSEGMENT, 0 );
  }
  else {
    if ( ( _bios_equiplist() & 0x30 ) == 0x30 ){
      enabled = false;
    }
    else{
      enabled = true;
      if ( useMcga ) {
        bufferLength = 64000;
        palette = (uint8_t *)malloc( 256*3 );
      }
      else if ( !useHercules ) {
        bufferLength = 16384;
        palette = (uint8_t *)malloc( 4*3 );
      }
      image = (uint8_t *)malloc( bufferLength );
    }
  }
}

void Graphics::returnToTextMode() {
  if ( useHercules ) {
    stop_hercules();
  }
  else {
    stop_graphics();
  }
  graphicsMode = false;
}

void Graphics::set_rgb(int16_t color_index, int16_t r, int16_t g, int16_t b) {
  if ( useMcga ) {
    if ( color_index > 255 ) {
      return;
    }
  }
  else if ( color_index > 3 ) {
    return;
  }
  r *= 64;
  g *= 64;
  b *= 64;
  r /= 100;
  g /= 100;
  b /= 100;
  color_index *= 3;
  palette[color_index] = r;
  palette[color_index + 1] = g;
  palette[color_index + 2] = b;
}
