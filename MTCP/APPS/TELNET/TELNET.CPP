/*

   mTCP Telnet.cpp
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


   Description: Telnet client

   Changes:

   2011-05-27: Initial release as open source software
   2011-10-01: Fix telnet options bug; was not checking to see
               if any free packets were available
   2011-10-25: Fix the above bug, again, correctly this time ...
               Check some more return codes, especially when
               allocating memory at startup.
               Minor restructuring of code.
               Ability to abort connection by pressing Esc,
               Ctrl-C or Ctrl-Break while waiting for it.
   2012-04-29: Add support for Xmodem and Ymodem
               Fix CR/LF handling once and for all (Add CR/NUL)
   2012-03-14: Add scroll region support; add addition non-CSI
               ESC commands.  Restructure and cleanup
   2015-01-18: Minor change to Ctrl-Break and Ctrl-C handling.
   2015-01-22: Add 132 column support
   2015-01-23: Bug fix; do a proper close and wait for all input
               to come in before exiting the main loop.  Shorten
               TCP close timeout; rewrite help text code to cut
               down on code bloat.
   2017-01-20: Bug fixes; When sending bytes make sure that data
               just don't get dropped because we ran out of outgoing
               packets.
   2018-10-27: Add code to eliminate CGA snow.  Add a CGA_MONO
               color scheme.
   2019-11-20: Terminal emulation bug fixes.
   2020-05-10: Extended keyboard support.
   2020-05-17: Mouse support.
   2020-05-18: Fixed bug with Unicode characters at end of buffer.
   2020-05-25: Added Sixel graphics support.
   2020-06-07: Fixed bugs with selection and made graphics more efficient.
   2020-06-13: Alternate code page support.
   2021-09-21: Added Unicode translation for vowels with macrons, added
               Hercules Graphics Card support, and changed the cursor to
               the default cursor.
   2021-10-13: Added printer support.

   Todo:

     Perf improvement: Minimize use of ScOffset in insline and delline
*/



#include <bios.h>
#include <dos.h>
#include <conio.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include CFG_H

#include "types.h"

#include "timer.h"
#include "trace.h"
#include "utils.h"
#include "packet.h"
#include "arp.h"
#include "tcp.h"
#include "tcpsockm.h"
#include "udp.h"
#include "dns.h"
#include "globals.h"
#include "telnet.h"
#include "telnetsc.h"
#include "keys.h"
#include "mouse.h"
#include "telnetsx.h"

#ifdef FILEXFER
#include "ymodem.h"
#endif

// Buffer lengths

#define NOTRACE
#define SERVER_NAME_MAXLEN   (80)
#define TCP_RECV_BUF_SIZE  (4096)
#define RECV_BUF_SIZE      (2048)
#define TERMTYPE_MAXLEN      (30)
#define PRINTER_INIT_SIZE    (17)
#define PRINT_COUNT           (8)

#define TELNET_CONNECT_TIMEOUT (30000ul)

// Globals: Server information

TcpSocket *mySocket;

char     serverAddrName[SERVER_NAME_MAXLEN];   // Target server name
IpAddr_t serverAddr;                           // Target server Ip address
uint16_t serverPort = 23;                      // Target server port (default is telnet)



SocketInputMode_t::e SocketInputMode = SocketInputMode_t::Telnet;
UserInputMode_t::e UserInputMode = UserInputMode_t::Telnet;




// Globals: Toggles and options

bool    DebugTelnet = false;   // Are we spitting out messages for telnet?
bool    DebugAnsi = false;     // Are we spitting out messages for ANSI codes?
bool    RawOrTelnet = true;    // Are we doing telnet or just raw?
uint8_t InitWrapMode = 1;      // Normally we wrap
uint8_t SendBsAsDel  = 1;
uint8_t LocalEcho    = 0;      // Is local echoing enabled?
uint8_t NewLineMode  = 4;      // 0 is CR/LF, 1 is CR, 2 is LF, 3 is CR/NUL and 4 is AUTO
uint8_t BackScrollPages = 4;
uint8_t PrinterInitialized = 0;

uint32_t ConnectTimeout = TELNET_CONNECT_TIMEOUT; // How many ms to wait for a connection

#ifdef USE_UTF
utfTrans defaultTransTable[NUMBER_OF_DEFAULT_UTF_TRANSLATIONS] = {
                                                                  { 0x0100, 0x41 },
                                                                  { 0x2500, 0xc4 },
                                                                  { 0x0101, 0x61 },
                                                                  { 0x2302, 0x7f },
                                                                  { 0x2502, 0xb3 },
                                                                  { 0x250c, 0xda },
                                                                  { 0x2310, 0xa9 },
                                                                  { 0x2510, 0xbf },
                                                                  { 0x2011, 0x2d },
                                                                  { 0x0112, 0x45 },
                                                                  { 0x2012, 0x2d },
                                                                  { 0x2212, 0x2d },
                                                                  { 0x0113, 0x65 },
                                                                  { 0x2013, 0x2d },
                                                                  { 0x2014, 0x2d },
                                                                  { 0x2514, 0xc0 },
                                                                  { 0x2018, 0x60 },
                                                                  { 0x2518, 0xd9 },
                                                                  { 0x2019, 0x27 },
                                                                  { 0x2219, 0xf9 },
                                                                  { 0x221a, 0xfb },
                                                                  { 0x201c, 0x22 },
                                                                  { 0x251c, 0xc3 },
                                                                  { 0x201d, 0x22 },
                                                                  { 0x1d11e, 0x26 },
                                                                  { 0x221e, 0xec },
                                                                  { 0x221f, 0x1c },
                                                                  { 0x2320, 0xf4 },
                                                                  { 0x2321, 0xf5 },
                                                                  { 0x2022, 0x07 },
                                                                  { 0x2524, 0xb4 },
                                                                  { 0x2026, 0x5f },
                                                                  { 0x2229, 0xef },
                                                                  { 0x012a, 0x49 },
                                                                  { 0x012b, 0x69 },
                                                                  { 0x252c, 0xc2 },
                                                                  { 0x2630, 0xf0 },
                                                                  { 0x2032, 0x27 },
                                                                  { 0x2033, 0x22 },
                                                                  { 0x2534, 0xc1 },
                                                                  { 0x2039, 0x3c },
                                                                  { 0x203a, 0x3e },
                                                                  { 0x263a, 0x01 },
                                                                  { 0x263b, 0x02 },
                                                                  { 0x203c, 0x13 },
                                                                  { 0x253c, 0xc5 },
                                                                  { 0x263c, 0x0f },
                                                                  { 0x2640, 0x0c },
                                                                  { 0x2642, 0x0b },
                                                                  { 0x2248, 0xf7 },
                                                                  { 0x014c, 0x4f },
                                                                  { 0x014d, 0x6f },
                                                                  { 0x2550, 0xcd },
                                                                  { 0x2551, 0xba },
                                                                  { 0x2552, 0xd5 },
                                                                  { 0x2553, 0xd6 },
                                                                  { 0x2554, 0xc9 },
                                                                  { 0x2555, 0xb8 },
                                                                  { 0x2556, 0xb7 },
                                                                  { 0x2557, 0xbb },
                                                                  { 0x2558, 0xd4 },
                                                                  { 0x2559, 0xd3 },
                                                                  { 0x255a, 0xc8 },
                                                                  { 0x255b, 0xbe },
                                                                  { 0x255c, 0xbd },
                                                                  { 0x255d, 0xbc },
                                                                  { 0x255e, 0xc6 },
                                                                  { 0x255f, 0xc7 },
                                                                  { 0x2560, 0xcc },
                                                                  { 0x2660, 0x06 },
                                                                  { 0x2261, 0xf0 },
                                                                  { 0x2561, 0xb5 },
                                                                  { 0x2562, 0xb6 },
                                                                  { 0x2563, 0xb9 },
                                                                  { 0x2663, 0x05 },
                                                                  { 0x2264, 0xf3 },
                                                                  { 0x2564, 0xd1 },
                                                                  { 0x2265, 0xf2 },
                                                                  { 0x2565, 0xd2 },
                                                                  { 0x2665, 0x03 },
                                                                  { 0x2566, 0xcb },
                                                                  { 0x2666, 0x04 },
                                                                  { 0x2567, 0xcf },
                                                                  { 0x2568, 0xd0 },
                                                                  { 0x2569, 0xca },
                                                                  { 0x016a, 0x55 },
                                                                  { 0x256a, 0xd8 },
                                                                  { 0x266a, 0x0d },
                                                                  { 0x016b, 0x75 },
                                                                  { 0x256b, 0xd7 },
                                                                  { 0x266b, 0x0e },
                                                                  { 0x256c, 0xce },
                                                                  { 0x207f, 0xfc },
                                                                  { 0x2580, 0xdf },
                                                                  { 0x2584, 0xdc },
                                                                  { 0x2588, 0xdb },
                                                                  { 0x258c, 0xdd },
                                                                  { 0x2190, 0x1b },
                                                                  { 0x2590, 0xde },
                                                                  { 0x2191, 0x18 },
                                                                  { 0x2591, 0xb0 },
                                                                  { 0x0192, 0x9f },
                                                                  { 0x2192, 0x1a },
                                                                  { 0x2592, 0xb1 },
                                                                  { 0x0393, 0xe2 },
                                                                  { 0x2193, 0x19 },
                                                                  { 0x2593, 0xb2 },
                                                                  { 0x2194, 0x1d },
                                                                  { 0x2195, 0x12 },
                                                                  { 0x0398, 0xe9 },
                                                                  { 0x00a0, 0xff },
                                                                  { 0x25a0, 0xfe },
                                                                  { 0x00a1, 0xad },
                                                                  { 0x00a2, 0x9b },
                                                                  { 0x00a3, 0x9c },
                                                                  { 0x03a3, 0xe4 },
                                                                  { 0x00a5, 0x9d },
                                                                  { 0x03a6, 0xe8 },
                                                                  { 0x00a7, 0x15 },
                                                                  { 0x20a7, 0x9e },
                                                                  { 0x00a9, 0x63 },
                                                                  { 0x03a9, 0xea },
                                                                  { 0x00aa, 0xa6 },
                                                                  { 0x00ab, 0xae },
                                                                  { 0x21ab, 0x17 },
                                                                  { 0x00ac, 0xaa },
                                                                  { 0x25ac, 0x16 },
                                                                  { 0x00ae, 0x52 },
                                                                  { 0x00b0, 0xf8 },
                                                                  { 0x00b1, 0xf1 },
                                                                  { 0x03b1, 0xe0 },
                                                                  { 0x00b2, 0xfd },
                                                                  { 0x25b2, 0x1e },
                                                                  { 0x03b4, 0xeb },
                                                                  { 0x00b5, 0xe6 },
                                                                  { 0x03b5, 0xee },
                                                                  { 0x00b6, 0x14 },
                                                                  { 0x00b7, 0xfa },
                                                                  { 0x00ba, 0xa7 },
                                                                  { 0x25ba, 0x10 },
                                                                  { 0x00bb, 0xaf },
                                                                  { 0x00bc, 0xac },
                                                                  { 0x25bc, 0x1f },
                                                                  { 0x00bd, 0xab },
                                                                  { 0x00bf, 0xa8 },
                                                                  { 0x03c0, 0xe3 },
                                                                  { 0x03c3, 0xe5 },
                                                                  { 0x00c4, 0x8e },
                                                                  { 0x03c4, 0xe7 },
                                                                  { 0x25c4, 0x11 },
                                                                  { 0x00c5, 0x8f },
                                                                  { 0x00c6, 0x92 },
                                                                  { 0x03c6, 0xed },
                                                                  { 0x00c7, 0x80 },
                                                                  { 0x00c9, 0x90 },
                                                                  { 0x25cb, 0x09 },
                                                                  { 0x25cf, 0x07 },
                                                                  { 0x00d1, 0xa5 },
                                                                  { 0x00d6, 0x99 },
                                                                  { 0x00d7, 0x78 },
                                                                  { 0x25d8, 0x08 },
                                                                  { 0x25d9, 0x0a },
                                                                  { 0x00dc, 0x9a },
                                                                  { 0x00df, 0xe1 },
                                                                  { 0x00e0, 0x85 },
                                                                  { 0x00e1, 0xa0 },
                                                                  { 0x00e2, 0x83 },
                                                                  { 0x00e4, 0x84 },
                                                                  { 0x00e5, 0x86 },
                                                                  { 0x00e6, 0x91 },
                                                                  { 0x00e7, 0x87 },
                                                                  { 0x00e8, 0x8a },
                                                                  { 0x00e9, 0x82 },
                                                                  { 0x00ea, 0x88 },
                                                                  { 0x00eb, 0x89 },
                                                                  { 0x00ec, 0x8d },
                                                                  { 0x00ed, 0xa1 },
                                                                  { 0x00ee, 0x8c },
                                                                  { 0x00ef, 0x8b },
                                                                  { 0x00f1, 0xa4 },
                                                                  { 0x00f2, 0x95 },
                                                                  { 0x00f3, 0xa2 },
                                                                  { 0x00f4, 0x93 },
                                                                  { 0x00f6, 0x94 },
                                                                  { 0x00f7, 0xf6 },
                                                                  { 0x00f9, 0x97 },
                                                                  { 0x00fa, 0xa3 },
                                                                  { 0x00fb, 0x96 },
                                                                  { 0x00fc, 0x81 },
                                                                  { 0x00ff, 0x98 }
};
#endif
char TermType[TERMTYPE_MAXLEN] = "ANSI";
//char *incomingClipboard = (char *)malloc(BASE64_BUFFER_SIZE);

static uint16_t processSocket( uint8_t *recvBuffer, uint16_t len );

static uint16_t processCSISeq( uint8_t *buffer, uint16_t len );
static uint16_t processOSCSeq( uint8_t *buffer, uint16_t len );
#ifdef USE_UTF
static uint16_t processUTFSeq( uint8_t *buffer, uint16_t len, uint16_t size );
#endif
inline void     processAnsiCommand( uint8_t commandLetter );
static void     processDecPrivateControl( uint8_t commandLetter );
static void     processNonCSIEscSeq( uint8_t ch );

       int16_t  processTelnetCmds( uint8_t *cmdStr, uint8_t cmdSize );
static int16_t  processSingleTelnetCmd( uint8_t *cmdStr, uint8_t inputBufLen, uint8_t *outputBuf, uint16_t *outputBufLen );

static uint8_t  processUserInput_TelnetLocal( Key_t key );
static void     processUserInput_TelnetNonLocal( Key_t key );

static void     parseArgs( int argc, char *argv[] );
static void     getCfgOpts( void );

static void     resolveAndConnect( void );
static void     sendInitialTelnetOpts( void );
static void     shutdown( int rc );

static void     doHelp( void );

static void     send( uint8_t *userBuf, uint16_t userBufLen );

#ifdef USE_UTF
char utf_chars[4];
uint16_t utf_chars_seen;
#endif
// char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// static inline bool is_base64(char c) {
//   return (isalnum(c) || (c == '+') || (c == '/'));
// }

// static void base64_decode(char *encoded_string, char *ret, int maxlen) {
//   int in_len = strlen(encoded_string);
//   int i = 0;
//   int j = 0;
//   int k = 0;
//   int in_ = 0;
//   int l;
//   char char_array_4[4], char_array_3[3];
//   maxlen--; // leave room for string terminator

//   while ( in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_]) ) {
//     char_array_4[i++] = encoded_string[in_];
//     in_++;
//     if (i == 4) {
//       for (i = 0; i < 4; i++) {
//         bool found = false;
//         for (l = 0; l < 64; ++l) {
//           if ( char_array_4[i] == base64_chars[l] ) {
//             char_array_4[i] = l;
//             found = true;
//             break;
//           }
//         }
//         if (!found) {
//           char_array_4[i] = 255;
//         }
//       }
//       char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
//       char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
//       char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

//       for (i = 0; (i < 3); i++) {
//         if (k < maxlen) {
//           ret[k++] = char_array_3[i];
//         }
//       }
//       i = 0;
//     }
//   }

//   if (i) {
//     for (j = i; j < 4; j++) {
//       char_array_4[j] = 0;
//     }
//     for (j = 0; j < 4; j++) {
//       bool found = false;
//       for (l = 0; l < 64; ++l) {
//         if ( char_array_4[j] == base64_chars[l] ) {
//           char_array_4[j] = l;
//           found = true;
//           break;
//         }
//       }
//       if (!found) {
//         char_array_4[j] = 255;
//       }
//     }
//     char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
//     char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
//     char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

//     for (j = 0; (j < i - 1); j++) {
//       if (k < maxlen) {
//         ret[k++] = char_array_3[j];
//       }
//     }
//   }
//   ret[k++] = 0;
// }

// Ctrl-Break and Ctrl-C handlers.

// Check this flag once in a while to see if the user wants out.
volatile uint8_t CtrlBreakDetected = 0;

void __interrupt __far ctrlBreakHandler( ) {
  CtrlBreakDetected = 1;
}

void __interrupt __far ctrlCHandler( ) {
  // Do Nothing - Ctrl-C is a legal character
}




// Telnet options negotiation variables

TelnetOpts MyTelnetOpts;


// Telnet opts for this program
//
//    Option             Remote   Local
//  0 Binary             on       on
//  1 Echo               on       off
//  3 SGA                on       on
//  5 Status             off      off
//  6 Timing mark        off      off
// 24 Terminal type      off      on
// 31 Window Size        off      on
// 32 Terminal speed     off      off
// 33 Remote Flow Ctrl   off      off
// 34 Linemode           off      off
// 35 X Display          off      off
// 36 Environment vars   off      off
// 39 New environment    off      off





// Screen handling and emulation

Screen s;
Graphics g;

#ifdef USE_UTF
enum StreamStates { Normal = 0, ESC_SEEN, CSI_SEEN, OSC_SEEN, DCS_SEEN, IAC_SEEN, UTF2_SEEN, UTF3_SEEN, UTF4_SEEN };
#else
enum StreamStates { Normal = 0, ESC_SEEN, CSI_SEEN, OSC_SEEN, DCS_SEEN, IAC_SEEN };
#endif
StreamStates StreamState;




// ANSI escape sequence handling globals.  These are global so that
// we can parse a partially filled buffer and resume later.

#define CSI_ARGS (16)
#define CSI_DEFAULT_ARG (-1)

enum CsiParseState_t { LookForPrivateControl, NoParmsFound, ParmsFound };
CsiParseState_t csiParseState;
enum OscParseState_t { LookForPs, LookForClipboardCode, LookForData };
OscParseState_t oscParseState;

int16_t  parms[ CSI_ARGS ];  // Array of parameters
uint16_t parmsFound;         // Number of parameters found
bool     decPrivateControl;  // Is this a private control sequence?
uint8_t  clipboardType;
uint8_t  cutBuffer;
bool     clipboardStore;
bool     needToSendSelection = false;

char     traceBuffer[60];    // ANSI debug trace buffer
uint16_t traceBufferLen;

uint8_t fg=7;
uint8_t bg=0;
uint8_t bold=0;
uint8_t blink=0;
bool    underline=false;
uint8_t reverse=0;

int16_t saved_cursor_x = 0, saved_cursor_y = 0;

uint8_t Strict_DOS_Mode = 0;


uint8_t ColorScheme = 0;  // 0 default, 1 = CGA_MONO

uint8_t *fgColorMap;
uint8_t *bgColorMap;
char printerInit[PRINTER_INIT_SIZE] = "";

// Input is ANSI, output is CGA Attribute
uint8_t fgColorMap_CGA[] = {
  0, // 0 - Black
  4, // 1 - Red,
  2, // 2 - Green,
  6, // 3 - Yellow
  1, // 4 - Blue
  5, // 5 - Magenta
  3, // 6 - Cyan
  7, // 7 - White
  7, // 8 - (undefined)
  7  // 9 - (reset to default)
};

uint8_t bgColorMap_CGA[] = {
  0, // 0 - Black
  4, // 1 - Red,
  2, // 2 - Green,
  6, // 3 - Yellow
  1, // 4 - Blue
  5, // 5 - Magenta
  3, // 6 - Cyan
  7, // 7 - White
  0, // 8 - (undefined)
  0  // 9 - (reset to default)
};

uint8_t fgColorMap_Mono[] = {
  0, // 0 - Black
  7, // 1 - Red,
  7, // 2 - Green,
  7, // 3 - Yellow
  7, // 4 - Blue
  7, // 5 - Magenta
  7, // 6 - Cyan
  7, // 7 - White
  7, // 8 - (undefined)
  7  // 9 - (reset to default)
};

uint8_t bgColorMap_Mono[] = {
  0, // 0 - Black
  0, // 1 - Red,
  0, // 2 - Green,
  0, // 3 - Yellow
  0, // 4 - Blue
  0, // 5 - Magenta
  0, // 6 - Cyan
  7, // 7 - White
  0, // 8 - (undefined)
  0  // 9 - (reset to default)
};


uint8_t scNormal;       // Normal text
uint8_t scBright;       // Bright/Bold
uint8_t scTitle;        // Title - used only at startup
uint8_t scBorder;       // Border lines on help window
uint8_t scCommandKey;   // Used in the help menu
uint8_t scToggleStatus; // Used in the help menu
uint8_t scFileXfer;     // File transfer dialog boxes
uint8_t scErr;          // Error messages
Mouse_t mouse;          // Mouse status

static char tmpBuf[160];


static char CopyrightMsg1[] = "mTCP Telnet by M Brutman (mbbrutman@gmail.com) (C)opyright 2009-2022\r\n";
static char CopyrightMsg2[] = "Version: " __DATE__ "\r\n\r\n";

extern "C" uint16_t cdecl initialize_printer( char far *initstring );

extern "C" uint8_t far mouse_changed;

extern "C" uint16_t far buffer_buttons[16];
extern "C" uint16_t far buffer_time_cx[16];
extern "C" uint16_t far buffer_time_dx[16];
extern "C" uint16_t far buffer_col[16];
extern "C" uint16_t far buffer_row[16];
extern "C" uint8_t far buffer_start;
extern "C" uint8_t far buffer_end;
uint8_t buffer_prev;
uint16_t button_change;
uint16_t buttons_down;
uint16_t buttons_up;
uint32_t ticks;
uint32_t prevPrevLeftDown;
uint32_t prevLeftDown;
uint32_t currentTime;
uint32_t currentTimeIsGood;
uint32_t lastMouseTime = 0;

void mouse_init( void ) {
  if (ibm_mouse_init() == -1) {
    //printf( "\r\nUsing mouse\r\n" );
    mouse.enable = true;
  }
  else {
    //printf( "\r\nNot using mouse\r\n" );
    mouse.enable = false;
  }
  mouse.showing = false;
  mouse.should_show = false;
  mouse.installed = false;
  mouse.buttons = 0;
  mouse.row = 0;
  mouse.column = 0;
  mouse.xterm_normal = false;
  mouse.xterm_motion = false;
  mouse.xterm_sgr = false;
  mouse.xterm_any = false;
  mouse.bracketed_paste = false;
  mouse.pending = 0;
  mouse.prev_down = 0;
  mouse.prev_buttons = 0;
  mouse.ignore_next_right_up = false;
  mouse.next_up_is_middle_up = false;
  mouse.ignore_next_left_up = false;
}

void mouse_install( void ) {
  if (mouse.enable && !mouse.installed) {
    ibm_mouse_install();
    mouse_changed = 0;
    mouse.installed = true;
  }
}

void mouse_uninstall( void ) {
  if (mouse.enable && mouse.installed) {
    ibm_mouse_uninstall();
    mouse_changed = 0;
    mouse.installed = false;
  }
}

void mouse_show( void ) {
  if (mouse.installed && !(mouse.showing) && !(s.usingGraphics)) {
    ibm_mouse_show();
    mouse.showing = true;
  }
}

void mouse_hide( void ) {
  if (mouse.installed && mouse.showing && !(s.usingGraphics)) {
    ibm_mouse_hide();
    mouse.showing = false;
  }
}

// void mouse_get_status( void ) {
//   if (mouse.enable) {
//     ibm_mouse_get_status(&(mouse.buttons), &(mouse.column), &(mouse.row));
//     mouse.column = mouse.column >> 3;
//     mouse.row = mouse.row >> 3;
//   }
// }

uint8_t prevRow = 255;
uint8_t prevCol = 255;
uint8_t downRow = 255;
uint8_t downCol = 255;

void send_move() {
  if ( s.usingGraphics ) return;
  if ( mouse.buttons == 0 || ( mouse.row == prevRow && mouse.column == prevCol ) ) {
    prevRow = mouse.row;
    prevCol = mouse.column;
    return;
  }
  prevRow = mouse.row;
  prevCol = mouse.column;
  if ( !mouse.xterm_any ) {
    if ( mouse.buttons & 3 ) {
      mouse_hide();
      s.extendSelectionRelative(mouse.row - 1, mouse.column - 1, downRow - 1, downCol - 1);
      s.updateHighlight();
      mouse_show();
    }
    return;
  }
  if ( mouse.xterm_sgr ) {
    if ( mySocket->outgoingQueueIsFull( ) ) return;
    uint8_t buttonVal = 32;
    DataBuf *buf = (DataBuf *)TcpBuffer::getXmitBuf( );
    if ( buf == NULL ) return;
    uint8_t i = 0;
    buf->data[i++] = 0x1b;
    buf->data[i++] = '[';
    buf->data[i++] = '<';
    if ( mouse.buttons & 1 ) {
      buttonVal += 0;
    }
    else if ( mouse.buttons & 2 ) {
      buttonVal += 2;
    }
    else if ( mouse.buttons & 4 ) {
      buttonVal += 1;
    }
    char Cb[3];
    int Cbn = sprintf(Cb, "%d", buttonVal);
    uint8_t j;
    for (j = 0; j < Cbn; j++) {
      buf->data[i++] = Cb[j];
    }
    buf->data[i++] = ';';
    char Px[3];
    char Py[3];
    int Pxn = sprintf(Px, "%d", mouse.row);
    int Pyn = sprintf(Py, "%d", mouse.column);
    for (j = 0; j < Pxn; j++) {
      buf->data[i++] = Px[j];
    }
    buf->data[i++] = ';';
    for (j = 0; j < Pyn; j++) {
      buf->data[i++] = Py[j];
    }
    buf->data[i++] = 'M';
    buf->b.dataLen = i;
    if ( mySocket->enqueue( &buf->b ) != 0 ) {
      TcpBuffer::returnXmitBuf( (TcpBuffer *)buf );
    }
  }
  else if ( mouse.xterm_normal ) {
    if ( mySocket->outgoingQueueIsFull( ) ) return;
    DataBuf *buf = (DataBuf *)TcpBuffer::getXmitBuf( );
    if ( buf == NULL ) return;
    uint8_t i = 0;
    uint8_t Cb;
    if ( mouse.buttons & 4 ) {
      Cb = 65;
    }
    if ( mouse.buttons & 2 ) {
      Cb = 66;
    }
    else {
      Cb = 64;
    }
    buf->b.dataLen = 6;
    buf->data[0] = 0x1b;
    buf->data[1] = '[';
    buf->data[2] = 'M';
    buf->data[3] = Cb;
    buf->data[4] = 32 + mouse.row;
    buf->data[5] = 32 + mouse.column;
    if ( mySocket->enqueue( &buf->b ) != 0 ) {
      TcpBuffer::returnXmitBuf( (TcpBuffer *)buf );
    }
  }
}

void send_selection() {
  if ( s.usingGraphics ) return;
  DataBuf *buf = (DataBuf *)TcpBuffer::getXmitBuf( );
  if ( buf == NULL ) return;
  int selection_length = strlen(s.selection.buf);
  int i = 0;
  if (mouse.bracketed_paste) {
    buf->data[i++] = 0x1b;
    buf->data[i++] = '[';
    buf->data[i++] = '2';
    buf->data[i++] = '0';
    buf->data[i++] = '0';
    buf->data[i++] = '~';
  }
  for (int j = 0; j < selection_length; ++j) {
    buf->data[i++] = s.selection.buf[j];
  }
  if (mouse.bracketed_paste) {
    buf->data[i++] = 0x1b;
    buf->data[i++] = '[';
    buf->data[i++] = '2';
    buf->data[i++] = '0';
    buf->data[i++] = '1';
    buf->data[i++] = '~';
  }
  buf->b.dataLen = i;
  if ( mySocket->enqueue( &buf->b ) != 0 ) {
    TcpBuffer::returnXmitBuf( (TcpBuffer *)buf );
  }
}

void send_click( uint8_t direction, uint8_t buttonVal ) {
  if ( mySocket->outgoingQueueIsFull( ) || s.usingGraphics ) return;
  if ( mouse.xterm_sgr ) {
    // if (buttonVal == XTERM_BUTTON_MIDDLE) {
    //   if (direction == MOUSE_UP) {
    //         send_selection();
    //   }
    // }
    // else {
    prevRow = mouse.row;
    prevCol = mouse.column;
    DataBuf *buf = (DataBuf *)TcpBuffer::getXmitBuf( );
    if ( buf == NULL ) return;
    uint8_t i = 0;
    buf->data[i++] = 0x1b;
    buf->data[i++] = '[';
    buf->data[i++] = '<';
    char Cb[3];
    int Cbn = sprintf(Cb, "%d", buttonVal);
    uint8_t j;
    for (j = 0; j < Cbn; j++) {
      buf->data[i++] = Cb[j];
    }
    buf->data[i++] = ';';
    char Px[3];
    char Py[3];
    int Pxn = sprintf(Px, "%d", mouse.row);
    int Pyn = sprintf(Py, "%d", mouse.column);
    for (j = 0; j < Pxn; j++) {
      buf->data[i++] = Px[j];
    }
    buf->data[i++] = ';';
    for (j = 0; j < Pyn; j++) {
      buf->data[i++] = Py[j];
    }
    if ( direction == MOUSE_DOWN ) {
      buf->data[i++] = 'M';
    }
    else {
      buf->data[i++] = 'm';
    }
    buf->b.dataLen = i;
    if ( mySocket->enqueue( &buf->b ) != 0 ) {
      TcpBuffer::returnXmitBuf( (TcpBuffer *)buf );
    }
    // }
  }
  else if ( mouse.xterm_normal ) {
    // if (buttonVal == XTERM_BUTTON_MIDDLE) {
    //   if (direction == MOUSE_UP) {
    //         send_selection();
    //   }
    // }
    // else {
    DataBuf *buf = (DataBuf *)TcpBuffer::getXmitBuf( );
    if ( buf == NULL ) return;
    uint8_t i = 0;
    uint8_t Cb;
    if ( direction == MOUSE_DOWN ) {
      Cb = 32 + buttonVal;
    }
    else {
      Cb = '#';
    }
    buf->b.dataLen = 6;
    buf->data[0] = 0x1b;
    buf->data[1] = '[';
    buf->data[2] = 'M';
    buf->data[3] = Cb;
    buf->data[4] = 32 + mouse.row;
    buf->data[5] = 32 + mouse.column;
    if ( mySocket->enqueue( &buf->b ) != 0 ) {
      TcpBuffer::returnXmitBuf( (TcpBuffer *)buf );
    }
    // }
  }
  else {
    if ( direction == XTERM_BUTTON_DOWN && buttonVal == XTERM_BUTTON_LEFT ) {
      downRow = mouse.row;
      downCol = mouse.column;
      if (!currentTimeIsGood) {
        ibm_mouse_get_time(&currentTime);
      }
      if ( currentTime - prevPrevLeftDown < 13) { // triple click
        mouse_hide();
        s.highlightSelection(false);
        s.readLine(mouse.column - 1);
        s.highlightSelection(true);
        mouse_show();
        prevPrevLeftDown = 0;
        prevLeftDown = 0;
      }
      else if ( currentTime - prevLeftDown < 10) { // double click
        mouse_hide();
        s.highlightSelection(false);
        s.readWord(mouse.row - 1, mouse.column - 1);
        s.highlightSelection(true);
        mouse_show();
        prevPrevLeftDown = prevLeftDown;
        prevLeftDown = currentTime;
      }
      else {
        mouse_hide();
        s.highlightSelection(false);
        s.setSelectionStart(mouse.row - 1, mouse.column - 1);
        mouse_show();
        prevPrevLeftDown = prevLeftDown;
        prevLeftDown = currentTime;
      }
    }
    if ( direction == XTERM_BUTTON_DOWN && buttonVal == XTERM_BUTTON_RIGHT ) {
      mouse_hide();
      s.highlightSelection(false);
      s.extendSelection(mouse.row - 1, mouse.column - 1);
      s.highlightSelection(true);
      mouse_show();
    }
    if ( direction == XTERM_BUTTON_UP && ( buttonVal == XTERM_BUTTON_RIGHT || buttonVal == XTERM_BUTTON_LEFT ) ) {
      if ( s.selection.start != s.selection.end ) {
        s.populateSelection();
      }
    }
    if ( direction == XTERM_BUTTON_UP && buttonVal == XTERM_BUTTON_MIDDLE ) {
      send_selection();
    }
  }
}

int main( int argc, char *argv[] ) {
  printf( "%s  %s", CopyrightMsg1, CopyrightMsg2 );

  parseArgs( argc, argv );

  // Initialize TCP/IP

  if ( Utils::parseEnv( ) != 0 ) {
    exit(-1);
  }

  g.useMcga = true;
  g.useHercules = false;
  getCfgOpts( );
  if ( Utils::initStack( 1, TCP_SOCKET_RING_SIZE, ctrlBreakHandler, ctrlCHandler ) ) {
    printf( "\nFailed to initialize TCP/IP - exiting\n" );
    exit(-1);
  }

  // From this point forward you have to call the shutdown( ) routine to
  // exit because we have the timer interrupt hooked.

  mouse_init();
  mouse_install();
  s.initializeSelection();
  g.initializeGraphics();
  if (get_extended_keyboard_cap() == 1){
    kKeyGet = 0x00;
    kKeyCheck = 0x01;
    kKeyFlags = 0x02;
  }
  else {
    kKeyGet = 0x10;
    kKeyCheck = 0x11;
    kKeyFlags = 0x12;
  }

  // Allocate memory for a receive buffer.  This is in addition to the normal
  // socket receive buffer.  Do this early so that we don't get too far into
  // the code before failing.

  uint8_t *recvBuffer = (uint8_t *)malloc( RECV_BUF_SIZE );

  if ( (recvBuffer == NULL) || s.init( BackScrollPages, InitWrapMode, g.useMcga ) ) {
    puts( "\nNot enough memory - exiting\n" );
    shutdown( -1 );
  }
  if ( !g.useMcga && !g.useHercules ){
    s.preventSnow = true;
  }

  #ifdef FILEXFER
  initForXmodem( );
  #endif


  // Cheap hack to change behavior without making yet another configuration parameter.

  if ( getenv("TELNET_STRICT_DOS") ) {
    Strict_DOS_Mode = 1;
  }




  if ( s.isColorCard( ) && (ColorScheme ==0) ) {
    fgColorMap = fgColorMap_CGA;
    bgColorMap = bgColorMap_CGA;
  }
  else {
    fgColorMap = fgColorMap_Mono;
    bgColorMap = bgColorMap_Mono;
  }



  // Set color palette up
  if ( s.isColorCard( ) ) {
    if ( ColorScheme == 0 ) {
      scNormal       = 0x07; // White on black
      scBright       = 0x0F; // Bright White on black
      scTitle        = 0x1F; // Bright White on blue
      scBorder       = 0x0C; // Bright Red on black
      scCommandKey   = 0x09; // Bright Blue on black
      scToggleStatus = 0x0E; // Yellow on black
      scFileXfer     = 0x1F; // Bright White on blue
      scErr          = 0x4F; // Red on blue
    } else {
      scNormal       = 0x07; // Normal
      scBright       = 0x0F; // Bright
      scTitle        = 0x0F; // Bright
      scBorder       = 0x0F; // Bright
      scCommandKey   = 0x70; // Reverse
      scToggleStatus = 0x0F; // Bright
      scFileXfer     = 0x0F; // Bright
      scErr          = 0x70; // Reverse
    }
  }
  else {
    scNormal       = 0x02; // Normal
    scBright       = 0x0F; // Bright
    scTitle        = 0x0F; // Bright
    scBorder       = 0x0F; // Bright
    scCommandKey   = 0x01; // Underlined
    scToggleStatus = 0x01; // Underlined
    scFileXfer     = 0x0F; // Bright
    scErr          = 0x70; // Reverse
  }



  s.curAttr = scTitle;
  s.add( CopyrightMsg1 );
  s.curAttr = scNormal;
  s.add( "  " );
  s.curAttr = scTitle;
  s.add( CopyrightMsg2 );
  s.curAttr = scNormal;

  resolveAndConnect( );

  s.add ( "Remember to use " );
  s.curAttr = scBright; s.add( "Alt-H" ); s.curAttr = scNormal;
  s.add( " for help!\r\n\r\n" );


  sprintf( tmpBuf, "Connected to %s (%d.%d.%d.%d) on port %u\r\n\r\n",
           serverAddrName, serverAddr[0], serverAddr[1],
           serverAddr[2], serverAddr[3], serverPort );
  s.add( tmpBuf );


  sendInitialTelnetOpts( );


  // done is more than a simple flag:
  //
  //   0 - everything is fine
  //   1 - we want to close or the other side has initiated a close
  //   2 - we started a close
  //   3 - the close is complete, it timed out, or the user is impatient.

  uint8_t done = 0;

  uint16_t bytesToRead = RECV_BUF_SIZE;
  uint16_t bytesInBuffer = 0;

  while ( done != 3 ) {

    if ( CtrlBreakDetected ) {
      if ( done == 0 ) done = 1; else done = 3;
    }

    PACKET_PROCESS_SINGLE;
    Arp::driveArp( );
    Tcp::drivePackets( );


    // Process incoming packets first.
    //
    // Loop and read from the socket and process what we receive until there
    // is no more to receive.

    // Make sure that we break out to process user input once in a while.
    uint16_t packetsProcessed = 10;

    while ( packetsProcessed ) {

      int16_t recvRc = mySocket->recv( recvBuffer+bytesInBuffer, bytesToRead-bytesInBuffer );

      PACKET_PROCESS_SINGLE;
      Arp::driveArp( );
      Tcp::drivePackets( );

      if ( recvRc > 0 ) {

        packetsProcessed--;

        bytesInBuffer += recvRc;

        if ( SocketInputMode == SocketInputMode_t::Telnet ) {
          bytesInBuffer = processSocket( recvBuffer, bytesInBuffer );
        }
        #ifdef FILEXFER
        else if ( SocketInputMode == SocketInputMode_t::Download ) {
          bytesInBuffer = processSocket_Download( recvBuffer, bytesInBuffer );
        }
        else if ( SocketInputMode == SocketInputMode_t::Upload ) {
          bytesInBuffer = processSocket_Upload( recvBuffer, bytesInBuffer );
        }
        #endif

      } else {
        break;
      }

    } // end while

    // We might have had bytes to process even though we have not received new
    // data.  Take care of them.  (This only should happen if we are processing
    // telnet options and we use up too many outgoing buffers with small payloads.)

    if ( bytesInBuffer ) {
      if ( SocketInputMode == SocketInputMode_t::Telnet ) {
        bytesInBuffer = processSocket( recvBuffer, bytesInBuffer );
      }
      #ifdef FILEXFER
      else if ( SocketInputMode == SocketInputMode_t::Download ) {
        bytesInBuffer = processSocket_Download( recvBuffer, bytesInBuffer );
      }
      else if ( SocketInputMode == SocketInputMode_t::Upload ) {
        bytesInBuffer = processSocket_Upload( recvBuffer, bytesInBuffer );
      }
      #endif
    }

    // If the other side closed on us start closing down our side.
    if ( mySocket->isRemoteClosed( ) && (done == 0) ) done = 1;


    #ifdef FILEXFER
    if ( SocketInputMode == SocketInputMode_t::Download ) {
      transferVars.checkForDownloadTimeout( );
    }
    #ifdef YMODEM_G
    else if ( (SocketInputMode == SocketInputMode_t::Upload) &&
              (transferVars.fileProtocol == Transfer_vars::Ymodem_G) &&
              (transferVars.packetState == Transfer_vars::Uploading) &&
              (bytesInBuffer == 0) ) {

      // Special code for Ymodem G.  It doesn't send an ACK after each packet
      // because it just expects us to keep sending packets.  If we don't
      // have any bytes received on the socket and we are doing a Ymodem G
      // upload then just force the next packet out.

      transferVars.sendForYmodemG( );
    }
    #endif
    #endif
    if ( s.PrintingAllowed && s.printerReadIndex != s.printerWriteIndex){
      s.writeCount = PRINT_COUNT;
      while (s.printerReadIndex != s.printerWriteIndex && s.writeCount-- > 0){
        if (++s.printerReadIndex >= PRINT_BUFFER_SIZE){
          s.printerReadIndex = 0;
        }
        send_char_to_printer(&(s.printerBuffer[s.printerReadIndex]));
      }
    }

    if ( s.isVirtualScreenUpdated( ) && UserInputMode == UserInputMode_t::Telnet ) {
      if ( mouse.xterm_any && mouse.showing ) {
        mouse_hide();
      }
      s.paint( );
      s.updateVidBufPtr( );
      if ( mouse.xterm_any && mouse.should_show ) {
        mouse_show();
      }
    }

    if ( UserInputMode == UserInputMode_t::Telnet ) { gotoxy( s.cursor_x, s.cursor_y ); }

    if ( mouse.installed ) {
      if ( needToSendSelection ) {
        send_selection();
        needToSendSelection = false;
      }
      if ( mouse.pending > 0 ) {
        ibm_mouse_get_time(&currentTime);
        currentTimeIsGood = true;
        ticks = currentTime - mouse.prev_down;
        if ( mouse.pending & MOUSE_LEFT ) {
          if ( ticks >= MOUSE_TICKS_MIDDLE_CLICK ) {
            send_click(XTERM_BUTTON_DOWN, XTERM_BUTTON_LEFT);
            mouse.buttons |= 1;
            mouse.pending &= ~MOUSE_LEFT;
            mouse.prev_down = 0;
          }
        }
        if ( mouse.pending & MOUSE_RIGHT ) {
          if ( ticks >= MOUSE_TICKS_MIDDLE_CLICK ) {
            send_click(XTERM_BUTTON_DOWN, XTERM_BUTTON_RIGHT);
            mouse.buttons |= 2;
            mouse.pending &= ~MOUSE_RIGHT;
            mouse.prev_down = 0;
          }
        }
      }
      else {
        currentTimeIsGood = false;
      }
      if ( mouse_changed ) {
        mouse_changed = 0;
        if ( !mouse.should_show ) {
          mouse.should_show = true;
          mouse_show();
        }
        if (currentTimeIsGood) {
          lastMouseTime = currentTime;
        }
        else {
          ibm_mouse_get_time(&lastMouseTime);
        }
        send_move();
        if ( buffer_start == buffer_end || ( mouse.xterm_any && mySocket->outgoingQueueIsFull( ) ) ) {
        }
        else {
          while (1) {
            if (buffer_start == 0) {
              buffer_prev = 15;
            }
            else {
              buffer_prev = buffer_start - 1;
            }
            if (mouse.pending && (buffer_row[buffer_start] != buffer_row[buffer_prev] || buffer_col[buffer_start] != buffer_col[buffer_prev]) && buffer_row[buffer_prev] != 255) {
              if ( mouse.pending & 1 ) {
                send_click(XTERM_BUTTON_DOWN, XTERM_BUTTON_LEFT);
                mouse.buttons |= 1;
                mouse.pending &= ~1;
              }
              if ( mouse.pending & 2 ) {
                send_click(XTERM_BUTTON_DOWN, XTERM_BUTTON_RIGHT);
                mouse.buttons |= 2;
                mouse.pending &= ~2;
              }
              send_move();
            }
            mouse.row = buffer_row[buffer_start] + 1;
            mouse.column = buffer_col[buffer_start] + 1;
            button_change = buffer_buttons[buffer_start] ^ buffer_buttons[buffer_prev];

            if ( button_change > 0 ) {
              buttons_down = buffer_buttons[buffer_start] & button_change;
              buttons_up = (~buffer_buttons[buffer_start]) & button_change;
              if ( buttons_down & 3 ) { // left and/or right down
                currentTime = (buffer_time_cx[buffer_start] << 16) + buffer_time_dx[buffer_start];
                ticks = currentTime - mouse.prev_down;

                // if ( ticks < MOUSE_TICKS_DOUBLE_CLICK && ( buttons_down & 1 ) && ( mouse.prev_buttons & 1 )) {
                //         // double click
                //   if ( mouse.pending & MOUSE_DOUBLE_START ) {
                //           mouse.pending &= ~MOUSE_DOUBLE_START;
                //         }
                //         //printf("DC down\r\n");
                //   send_click(XTERM_BUTTON_DOWN, XTERM_BUTTON_LEFT);
                //         mouse.buttons |= 1;
                //         mouse.prev_down = 0;
                //         mouse.pending &= ~1;
                // }
                if ( ( ( buttons_down & 3 ) == 3 ) || ( ticks < MOUSE_TICKS_MIDDLE_CLICK && (( buttons_down ^ mouse.prev_buttons ) & 3 ) == 3 ) ) {
                  // middle click
                  // send middle down
                  //printf("M DOWN\r\n");
                  send_click(XTERM_BUTTON_DOWN, XTERM_BUTTON_MIDDLE);
                  mouse.buttons |= 4;
                  //final_buttons_down &= ~3; // don't send
                  //final_buttons_down |= 4; // artificial middle
                  mouse.next_up_is_middle_up = true;
                  mouse.prev_down = 0;
                  mouse.pending &= ~3;
                }
                else {
                  mouse.pending |= (buttons_down & 3);
                  mouse.prev_down = currentTime;
                }
                mouse.prev_buttons = buttons_down;
              }
              if ( buttons_down & MOUSE_MIDDLE ) {
                //printf("M DOWN NATURAL\r\n");
                send_click(XTERM_BUTTON_DOWN, XTERM_BUTTON_MIDDLE);
                mouse.buttons |= 4;
              }
              if ( (buttons_up & 3) == 3 ) {
                //printf("M UP\r\n");
                if ( mouse.pending & 1 ) {
                  send_click(XTERM_BUTTON_DOWN, XTERM_BUTTON_LEFT);
                  mouse.buttons |= 1;
                  mouse.pending &= ~1;
                }
                if ( mouse.pending & 2 ) {
                  send_click(XTERM_BUTTON_DOWN, XTERM_BUTTON_RIGHT);
                  mouse.buttons |= 2;
                  mouse.pending &= ~2;
                }
                if ( mouse.buttons & 4 ) {
                  send_click(XTERM_BUTTON_UP, XTERM_BUTTON_MIDDLE);
                }
                else {
                  send_click(XTERM_BUTTON_UP, XTERM_BUTTON_LEFT);
                  send_click(XTERM_BUTTON_UP, XTERM_BUTTON_RIGHT);
                }
                mouse.buttons &= ~7;
                mouse.next_up_is_middle_up = false;
              }
              else {
                if ( buttons_up & MOUSE_LEFT ) {
                  // currentTime = (buffer_time_cx[buffer_start] << 16) + buffer_time_dx[buffer_start];
                  // ticks = currentTime - mouse.prev_down;
                  // if ( ticks < 9) {
                  //   mouse.pending |= MOUSE_DOUBLE_START;
                  // }
                  // else {
                  if ( mouse.pending & 1 ) {
                    //printf("L DOWN (ACCEL)\r\n");
                    send_click(XTERM_BUTTON_DOWN, XTERM_BUTTON_LEFT);
                    mouse.buttons |= 1;
                    mouse.pending &= ~1;
                  }
                  if (mouse.ignore_next_left_up) {
                    mouse.ignore_next_left_up = false;
                  }
                  else if (mouse.next_up_is_middle_up) {
                    //printf("M UP\r\n");
                    send_click(XTERM_BUTTON_UP, XTERM_BUTTON_MIDDLE);
                    mouse.buttons &= ~4;
                    mouse.next_up_is_middle_up = false;
                    mouse.ignore_next_right_up = true;
                  }
                  else {
                    //printf("L UP\r\n");
                    send_click(XTERM_BUTTON_UP, XTERM_BUTTON_LEFT);
                    mouse.buttons &= ~1;
                  }
                }
                if ( buttons_up & MOUSE_RIGHT ) {
                  if ( mouse.pending & 2 ) {
                    //printf("R DOWN (ACCEL)\r\n");
                    send_click(XTERM_BUTTON_DOWN, XTERM_BUTTON_RIGHT);
                    mouse.buttons |= 2;
                    mouse.pending &= ~2;
                  }
                  if (mouse.ignore_next_right_up) {
                    mouse.ignore_next_right_up = false;
                  }
                  else if (mouse.next_up_is_middle_up) {
                    //printf("M UP\r\n");
                    send_click(XTERM_BUTTON_UP, XTERM_BUTTON_MIDDLE);
                    mouse.buttons &= ~4;
                    mouse.next_up_is_middle_up = false;
                    mouse.ignore_next_left_up = true;
                  }
                  else {
                    //printf("R UP\r\n");
                    send_click(XTERM_BUTTON_UP, XTERM_BUTTON_RIGHT);
                    mouse.buttons &= ~2;
                  }
                }
              }
              if ( buttons_up & MOUSE_MIDDLE ) {
                //printf("M UP NATURAL\r\n");
                send_click(XTERM_BUTTON_UP, XTERM_BUTTON_MIDDLE);
                mouse.buttons &= ~4;
              }
            }

            buffer_start = (buffer_start + 1) & 0x0f;
            if (buffer_start == buffer_end || mouse.xterm_any) {
              break;
            }
          }
        }
      }
      else {
        if (!currentTimeIsGood) {
          ibm_mouse_get_time(&currentTime);
        }
        if ( ( currentTime - lastMouseTime ) > MOUSE_TICKS_SHOW_DURATION ) {
          mouse.should_show = false;
          mouse_hide();
        }
      }
    }

    // If a keystroke is waiting process it

    if ( bioskey(kKeyCheck) ) {
      Key_t key = getKey( );

      if ( s.usingGraphics ){
        g.returnToTextMode();
        s.usingGraphics = false;
        s.paint();
        mouse.should_show = false;
        mouse_hide();
      }
      else {
        if ( key.specialKey != K_NoKey ) {

          switch ( UserInputMode ) {

            case UserInputMode_t::Telnet:
              if ( key.local ) {
                // Returns 1 if the user wants to quit.
                if ( processUserInput_TelnetLocal( key ) ) {
                  if ( done == 0 ) done = 1; else done = 3;
                }
              } else {
                processUserInput_TelnetNonLocal( key );
              }
              break;

            case UserInputMode_t::Help:
              UserInputMode = UserInputMode_t::Telnet;
              s.paint( );
              s.updateVidBufPtr( );
              break;

            #ifdef FILEXFER

            case UserInputMode_t::ProtocolSelect_Download:
            case UserInputMode_t::ProtocolSelect_Upload:
              processUserInput_FileProtocol( key );
              break;

            case UserInputMode_t::FilenameSelect_Download:
            case UserInputMode_t::FilenameSelect_Upload:
              processUserInput_Filename( key );
              break;

            case UserInputMode_t::ClobberDialog:
            case UserInputMode_t::ClobberDialogDownloading:
              processUserInput_ClobberDialog( key );
              break;

            case UserInputMode_t::TransferInProgress:
              processUserInput_Transferring( key );
              break;

            #endif

          } // end switch
        }
      }
    } // end if keyboard pressed


    // The other side closed or the user requested an exit.  We want to do a
    // controlled close of the socket while processing any remaining incoming
    // data.  But there has to be a reasonable timeout period.

    if ( done ) {
      if ( done == 1 ) {
        mySocket->closeNonblocking( );
        done = 2;
      }
      else {
        if ( mySocket->isCloseDone( ) ) {
          done = 3;
        }
      }
    }
  }

  mouse_hide();
  s.curAttr = 0x07;
  s.add( "\r\nConnection closed - have a great day!\r\n" );

  TcpSocketMgr::freeSocket( mySocket );

  shutdown( 0 );

  return 0;
}

// This version of send is safe in that it checks the return code from the
// real socket send and loops if necessary to make sure the packet gets out.
//
// If the socket is dead at the time of this call nothing will happen.
// We don't bother passing a return code back; the other loops in the program
// will detect the dead socket soon enough.

void send( uint8_t *userBuf, uint16_t userBufLen ) {

  uint16_t bytesSent = 0;

  while ( bytesSent < userBufLen ) {

    int16_t rc = mySocket->send( userBuf+bytesSent, userBufLen-bytesSent );

    if ( rc < 0 ) break;

    bytesSent += rc;

    if ( rc == 0 ) {

      // We had data to send but none was sent; we must be backlogged.  Try
      // to process some packets.

      PACKET_PROCESS_SINGLE;
      Arp::driveArp( );
      Tcp::drivePackets( );
    }

  } // end while

  // Drive packets out at the end to speed things up.
  PACKET_PROCESS_SINGLE;
  Arp::driveArp( );
  Tcp::drivePackets( );
}

static void toggleOnSound( void ) {
  sound(500); delay(50); sound(750); delay(50); nosound( );
}

static void toggleOffSound( void ) {
  sound(500); delay(50); nosound( );
}

uint8_t processUserInput_TelnetLocal( Key_t key ) {

  if ( key.specialKey == K_Local_PageUp ) {
    s.paint( s.terminalLines );
  }
  else if ( key.specialKey == K_Local_PageDown ) {
    s.paint( 0 - s.terminalLines );
  }
  else if ( key.specialKey == K_Alt_R ) {
    s.clearConsole( ); // Flash the screen so they know we did something
    s.paint( );
  }
  else if ( key.specialKey == K_Alt_W ) {
    s.toggleWrapMode( );
    if ( s.isWrapModeOn( ) ) {
      toggleOnSound( );
    }
    else {
      toggleOffSound( );
    }
  }
  else if ( key.specialKey == K_Alt_E ) {
    LocalEcho = !LocalEcho;
    if ( LocalEcho ) {
      toggleOnSound( );
    }
    else {
      toggleOffSound( );
    }
  }
  else if ( key.specialKey == K_Alt_P ) {
    s.PrintingAllowed = !s.PrintingAllowed;
    if ( s.PrintingAllowed ) {
      toggleOnSound( );
      if ( !PrinterInitialized ){
        s.printerBuffer = (char *)malloc( PRINT_BUFFER_SIZE );
        s.printerReadIndex = 0;
        s.printerWriteIndex = 0;
        initialize_printer(printerInit);
        //s.printerFile = NULL;
        PrinterInitialized = 1;
      }
    }
    else {
      s.AutoPrint = 0;
      toggleOffSound( );
    }
  }
  else if ( key.specialKey == K_Alt_N ) {
    NewLineMode++;
    if ( NewLineMode == 5 ) NewLineMode = 0;
    toggleOnSound( );
  }
  else if ( key.specialKey == K_Alt_B ) {
    SendBsAsDel = !SendBsAsDel;
    if ( SendBsAsDel ) {
      toggleOnSound( );
    }
    else {
      toggleOffSound( );
    }
  }
  else if ( key.specialKey == K_Alt_H ) {
    doHelp( );
  }
  else if ( key.specialKey == K_Alt_X ) {
    return 1;
  }
  #ifdef FILEXFER
  else if ( key.specialKey == K_Alt_D ) {
    drawProtocolMenu( );
    UserInputMode  = UserInputMode_t::ProtocolSelect_Download;
    s.doNotUpdateRealScreen( );
  }
  else if ( key.specialKey == K_Alt_U ) {
    drawProtocolMenu( );
    UserInputMode  = UserInputMode_t::ProtocolSelect_Upload;
    s.doNotUpdateRealScreen( );
    // This space for rent.
  }
  else if ( key.specialKey == K_Alt_F ) {
    s.clearConsole( );
    s.myCprintf( 0, 0, scTitle, "mTCP Telnet DOS Shell" );
    s.myCprintf( 0, 2, scErr, "Warning! TCP/IP packets are not being processed.  Do not take too long or your" );
    s.myCprintf( 0, 3, scErr, "connection may be dropped!" );
    s.myCprintf( 0, 5, scNormal, "Use the \"exit\" command to return.\r\n\r\n" );
    system( "command" );
    s.paint( );
  }
  #endif

  return 0;
}



// This code uses a primitive method to send data through a socket; instead of
// calling the higher level send call which does memory copies, it grabs an
// outgoing transmission buffer directly and fills in the payload.  This cuts
// down on memcpy calls and overhead.
//
// It's gross but it performs better.  Everywhere else uses send and deals with
// the overhead because it is easier and those paths are not performance
// sensitive at all.

void processUserInput_TelnetNonLocal( Key_t key ) {

  // Do a quick pre-check to make sure that we have room in the outgoing packet
  // queue to send something.  Otherwise, there is no point in reading the
  // keyboard.

  if ( mySocket->outgoingQueueIsFull( ) ) return;


  // In theory we can send a packet if we need to.  Now get a packet ...  and
  // if we can't, exit early for the same reason.

  DataBuf *buf = (DataBuf *)TcpBuffer::getXmitBuf( );
  if ( buf == NULL ) return;


  buf->b.dataLen = 0;

  uint8_t sk = key.specialKey;

  switch ( sk ) {

    case K_NormalKey: {

      if ( SendBsAsDel ) {
        if ( key.normalKey == 8 ) {
          key.normalKey = 127;
        }
        else if ( key.normalKey == 127 ) {
          key.normalKey = 8;
        }
      }
      if ( key.normalKey == 0x20 ) {
        uint16_t flags = bioskey(kKeyFlags);
        if ( flags & 0x04 ) {
          buf->b.dataLen = 1;
          buf->data[0] = 0x00;
          break;
        }
      }

      buf->b.dataLen = 1;
      buf->data[0] = key.normalKey;
      break;

    }

    case K_AltKey: {
      buf->b.dataLen = 2;
      buf->data[0] = 0x1b; buf->data[1] = key.normalKey;
      break;
    }

    case K_Enter: {

      switch ( NewLineMode ) {
        case 0: {
          buf->b.dataLen = 2;
          buf->data[0] = 0x0D; buf->data[1] = 0x0A;
          break;
        }
        case 1: {
          buf->b.dataLen = 1;
          buf->data[0] = 0x0D;
          break;
        }
        case 2: {
          buf->b.dataLen = 1;
          buf->data[0] = 0x0A;
          break;
        }
        case 3: {
          buf->b.dataLen = 2;
          buf->data[0] = 0x0D; buf->data[1] = 0x00;
          break;
        }
        case 4: {
          if ( MyTelnetOpts.isLclOn( TELOPT_BIN ) ) {
            // Send just a CR
            buf->b.dataLen = 1;
            buf->data[0] = 0x0D;
          }
          else {
            // Send CR/NUL
            buf->b.dataLen = 2;
            buf->data[0] = 0x0D; buf->data[1] = 0x00;
          }
          break;
        }

      }

      break;
    }

    case K_Backtab: {
      buf->b.dataLen = 3;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'Z';
      break;
    }

    case K_CenterKey: {
      buf->b.dataLen = 3;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = 'E';
      break;
    }

    case K_Insert: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = ';'; buf->data[4] = '2'; buf->data[5] = '~';
        break;
      }
      buf->b.dataLen = 3;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'L';
      break;
    }

    case K_Home: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '2'; buf->data[5] = 'H';
        break;
      }
      buf->b.dataLen = 3;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'H';
      break;
    }

    case K_End: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '2'; buf->data[5] = 'F';
        break;
      }
      buf->b.dataLen = 3;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'F';
      break;
    }

    case K_CursorUp: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '2'; buf->data[5] = 'A';
        break;
      }
      buf->b.dataLen = 3;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'A';
      break;
    }

    case K_CursorDown: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '2'; buf->data[5] = 'B';
        break;
      }
      buf->b.dataLen = 3;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'B';
      break;
    }

    case K_CursorLeft: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '2'; buf->data[5] = 'D';
        break;
      }
      buf->b.dataLen = 3;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'D';
      break;
    }

    case K_CursorRight: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '2'; buf->data[5] = 'C';
        break;
      }
      buf->b.dataLen = 3;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = 'C';
      break;
    }

    case K_PageDown: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '6'; buf->data[3] = ';'; buf->data[4] = '2'; buf->data[5] = '~';
        break;
      }
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '6'; buf->data[3] = '~';
      break;
    }

    case K_PageUp: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '5'; buf->data[3] = ';'; buf->data[4] = '2'; buf->data[5] = '~';
        break;
      }
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '5'; buf->data[3] = '~';
      break;
    }

    case K_Delete: {

      // Linux doesn't have a key mapping for DEL in the ansi terminal
      // type.  You might want to send the xterm key mapping (shown in the
      // comment) but this doesn't work universally.  So just send a del
      // character now, which is the correct safe choice.  (Previously
      // it was a dead key, so this is a slight improvement.
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '3'; buf->data[3] = ';'; buf->data[4] = '2'; buf->data[5] = '~';
        break;
      }

      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '3'; buf->data[3] = '~';

      // buf->b.dataLen = 1;
      // buf->data[0] = 127;
      break;
    }

    case K_Ctrl_Insert: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = ';'; buf->data[4] = '6'; buf->data[5] = '~';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = ';'; buf->data[4] = '5'; buf->data[5] = '~';
      break;
    }

    case K_Ctrl_Home: {
      s.add( "" );
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '6'; buf->data[5] = 'H';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '5'; buf->data[5] = 'H';
      break;
    }

    case K_Ctrl_End: {
      s.add( "" );
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '6'; buf->data[5] = 'F';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '5'; buf->data[5] = 'F';
      break;
    }

    case K_Ctrl_CursorUp: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '6'; buf->data[5] = 'A';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '5'; buf->data[5] = 'A';
      break;
    }

    case K_Ctrl_CursorDown: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '6'; buf->data[5] = 'B';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '5'; buf->data[5] = 'B';
      break;
    }

    case K_Ctrl_CursorLeft: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '6'; buf->data[5] = 'D';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '5'; buf->data[5] = 'D';
      break;
    }

    case K_Ctrl_CursorRight: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '6'; buf->data[5] = 'C';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '5'; buf->data[5] = 'C';
      break;
    }

    case K_Ctrl_PageDown: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '6'; buf->data[3] = ';'; buf->data[4] = '6'; buf->data[5] = '~';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '6'; buf->data[3] = ';'; buf->data[4] = '5'; buf->data[5] = '~';
      break;
    }

    case K_Ctrl_PageUp: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '5'; buf->data[3] = ';'; buf->data[4] = '6'; buf->data[5] = '~';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '5'; buf->data[3] = ';'; buf->data[4] = '5'; buf->data[5] = '~';
      break;
    }

    case K_Ctrl_PrtScr: {
      buf->b.dataLen = 5;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '9'; buf->data[4] = '~';
      break;
    }

    case K_Alt_Insert: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = ';'; buf->data[4] = '4'; buf->data[5] = '~';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = ';'; buf->data[4] = '3'; buf->data[5] = '~';
      break;
    }

    case K_Alt_Home: {
      s.add( "" );
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '4'; buf->data[5] = 'H';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '3'; buf->data[5] = 'H';
      break;
    }

    case K_Alt_End: {
      s.add( "" );
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '4'; buf->data[5] = 'F';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '3'; buf->data[5] = 'F';
      break;
    }

    case K_Alt_CursorUp: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '4'; buf->data[5] = 'A';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '3'; buf->data[5] = 'A';
      break;
    }

    case K_Alt_CursorDown: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '4'; buf->data[5] = 'B';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '3'; buf->data[5] = 'B';
      break;
    }

    case K_Alt_CursorLeft: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '4'; buf->data[5] = 'D';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '3'; buf->data[5] = 'D';
      break;
    }

    case K_Alt_CursorRight: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '4'; buf->data[5] = 'C';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = ';'; buf->data[4] = '3'; buf->data[5] = 'C';
      break;
    }

    case K_Ctrl_Backspace: {
      //buf->b.dataLen = 1;
      //buf->data[0] = 0x08;
      s.add( "" );
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '7'; buf->data[3] = ';'; buf->data[4] = '5'; buf->data[5] = '~';
      break;
    }

    case K_F1: {
      buf->b.dataLen = 3;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = 'P';
      break;
    }

    case K_F2: {
      buf->b.dataLen = 3;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = 'Q';
      break;
    }

    case K_F3: {
      buf->b.dataLen = 3;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = 'R';
      break;
    }

    case K_F4: {
      buf->b.dataLen = 3;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = 'S';
      break;
    }

    case K_F5: {
      buf->b.dataLen = 5;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '5'; buf->data[4] = '~';
      break;
    }

    case K_F6: {
      buf->b.dataLen = 5;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '7'; buf->data[4] = '~';
      break;
    }

    case K_F7: {
      buf->b.dataLen = 5;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '8'; buf->data[4] = '~';
      break;
    }

    case K_F8: {
      buf->b.dataLen = 5;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '9'; buf->data[4] = '~';
      break;
    }

    case K_F9: {
      buf->b.dataLen = 5;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '0'; buf->data[4] = '~';
      break;
    }

    case K_F10: {
      buf->b.dataLen = 5;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '1'; buf->data[4] = '~';
      break;
    }

    case K_F11: {
      buf->b.dataLen = 5;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '3'; buf->data[4] = '~';
      break;
    }

    case K_F12: {
      buf->b.dataLen = 5;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '4'; buf->data[4] = '~';
      break;
    }

    case K_Shift_F1: {
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '2'; buf->data[3] = 'P';
      break;
    }

    case K_Shift_F2: {
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '2'; buf->data[3] = 'Q';
      break;
    }

    case K_Shift_F3: {
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '2'; buf->data[3] = 'R';
      break;
    }

    case K_Shift_F4: {
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '2'; buf->data[3] = 'S';
      break;
    }

    case K_Shift_F5: {
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '5'; buf->data[4] = ';'; buf->data[5] = '2'; buf->data[6] = '~';
      break;
    }

    case K_Shift_F6: {
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '7'; buf->data[4] = ';'; buf->data[5] = '2'; buf->data[6] = '~';
      break;
    }

    case K_Shift_F7: {
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '8'; buf->data[4] = ';'; buf->data[5] = '2'; buf->data[6] = '~';
      break;
    }

    case K_Shift_F8: {
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '9'; buf->data[4] = ';'; buf->data[5] = '2'; buf->data[6] = '~';
      break;
    }

    case K_Shift_F9: {
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '0'; buf->data[4] = ';'; buf->data[5] = '2'; buf->data[6] = '~';
      break;
    }

    case K_Shift_F10: {
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '1'; buf->data[4] = ';'; buf->data[5] = '2'; buf->data[6] = '~';
      break;
    }

    case K_Shift_F11: {
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '3'; buf->data[4] = ';'; buf->data[5] = '2'; buf->data[6] = '~';
      break;
    }

    case K_Shift_F12: {
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '4'; buf->data[4] = ';'; buf->data[5] = '2'; buf->data[6] = '~';
      break;
    }

    case K_Alt_F1: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 4;
        buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '4'; buf->data[3] = 'P';
        break;
      }
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '3'; buf->data[3] = 'P';
      break;
    }

    case K_Alt_F2: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 4;
        buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '4'; buf->data[3] = 'Q';
        break;
      }
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '3'; buf->data[3] = 'Q';
      break;
    }

    case K_Alt_F3: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 4;
        buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '4'; buf->data[3] = 'R';
        break;
      }
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '3'; buf->data[3] = 'R';
      break;
    }

    case K_Alt_F4: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 4;
        buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '4'; buf->data[3] = 'S';
        break;
      }
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '3'; buf->data[3] = 'S';
      break;
    }

    case K_Alt_F5: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '5'; buf->data[4] = ';'; buf->data[5] = '4'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[1] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '5'; buf->data[4] = ';'; buf->data[5] = '3'; buf->data[6] = '~';
      break;
    }

    case K_Alt_F6: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '7'; buf->data[4] = ';'; buf->data[5] = '4'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[1] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '7'; buf->data[4] = ';'; buf->data[5] = '3'; buf->data[6] = '~';
      break;
    }

    case K_Alt_F7: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '8'; buf->data[4] = ';'; buf->data[5] = '4'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[1] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '8'; buf->data[4] = ';'; buf->data[5] = '3'; buf->data[6] = '~';
      break;
    }

    case K_Alt_F8: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '9'; buf->data[4] = ';'; buf->data[5] = '4'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[1] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '9'; buf->data[4] = ';'; buf->data[5] = '3'; buf->data[6] = '~';
      break;
    }

    case K_Alt_F9: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '0'; buf->data[4] = ';'; buf->data[5] = '4'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[1] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '0'; buf->data[4] = ';'; buf->data[5] = '3'; buf->data[6] = '~';
      break;
    }

    case K_Alt_F10: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '1'; buf->data[4] = ';'; buf->data[5] = '4'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[1] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '1'; buf->data[4] = ';'; buf->data[5] = '3'; buf->data[6] = '~';
      break;
    }

    case K_Alt_F11: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '3'; buf->data[4] = ';'; buf->data[5] = '4'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[1] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '3'; buf->data[4] = ';'; buf->data[5] = '3'; buf->data[6] = '~';
      break;
    }

    case K_Alt_F12: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '4'; buf->data[4] = ';'; buf->data[5] = '4'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[1] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '4'; buf->data[4] = ';'; buf->data[5] = '3'; buf->data[6] = '~';
      break;
    }

    case K_Ctrl_F1: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 4;
        buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '6'; buf->data[3] = 'P';
        break;
      }
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '5'; buf->data[3] = 'P';
      break;
    }

    case K_Ctrl_F2: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 4;
        buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '6'; buf->data[3] = 'Q';
        break;
      }
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '5'; buf->data[3] = 'Q';
      break;
    }

    case K_Ctrl_F3: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 4;
        buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '6'; buf->data[3] = 'R';
        break;
      }
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '5'; buf->data[3] = 'R';
      break;
    }

    case K_Ctrl_F4: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 4;
        buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '6'; buf->data[3] = 'S';
        break;
      }
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = 'O'; buf->data[2] = '5'; buf->data[3] = 'S';
      break;
    }

    case K_Ctrl_F5: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '5'; buf->data[4] = ';'; buf->data[5] = '6'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '5'; buf->data[4] = ';'; buf->data[5] = '5'; buf->data[6] = '~';
      break;
    }

    case K_Ctrl_F6: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '5'; buf->data[4] = ';'; buf->data[5] = '6'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '7'; buf->data[4] = ';'; buf->data[5] = '5'; buf->data[6] = '~';
      break;
    }

    case K_Ctrl_F7: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '8'; buf->data[4] = ';'; buf->data[5] = '6'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '8'; buf->data[4] = ';'; buf->data[5] = '5'; buf->data[6] = '~';
      break;
    }

    case K_Ctrl_F8: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '9'; buf->data[4] = ';'; buf->data[5] = '6'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '1'; buf->data[3] = '9'; buf->data[4] = ';'; buf->data[5] = '5'; buf->data[6] = '~';
      break;
    }

    case K_Ctrl_F9: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '0'; buf->data[4] = ';'; buf->data[5] = '6'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '0'; buf->data[4] = ';'; buf->data[5] = '5'; buf->data[6] = '~';
      break;
    }

    case K_Ctrl_F10: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '1'; buf->data[4] = ';'; buf->data[5] = '6'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '1'; buf->data[4] = ';'; buf->data[5] = '5'; buf->data[6] = '~';
      break;
    }

    case K_Ctrl_F11: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '3'; buf->data[4] = ';'; buf->data[5] = '6'; buf->data[6] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '3'; buf->data[4] = ';'; buf->data[5] = '5'; buf->data[6] = '~';
      break;
    }

    case K_Ctrl_F12: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 7;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '4'; buf->data[4] = ';'; buf->data[5] = '6'; buf->data[7] = '~';
        break;
      }
      buf->b.dataLen = 7;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '2'; buf->data[3] = '4'; buf->data[4] = ';'; buf->data[5] = '5'; buf->data[6] = '~';
      break;
    }

    case K_Ctrl_Tab: {
      buf->b.dataLen = 4;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '3'; buf->data[3] = '~';
      break;
    }

    case K_Ctrl_Delete: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '3'; buf->data[3] = ';'; buf->data[4] = '6'; buf->data[5] = '~';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '3'; buf->data[3] = ';'; buf->data[4] = '5'; buf->data[5] = '~';
      break;
    }

    case K_Alt_Delete: {
      uint16_t flags = bioskey(kKeyFlags);
      if ( flags & 0x03 ) {
        buf->b.dataLen = 6;
        buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '3'; buf->data[3] = ';'; buf->data[4] = '4'; buf->data[5] = '~';
        break;
      }
      buf->b.dataLen = 6;
      buf->data[0] = 0x1b; buf->data[1] = '['; buf->data[2] = '3'; buf->data[3] = ';'; buf->data[4] = '3'; buf->data[5] = '~';
      break;
    }

  } // end switch

  if ( LocalEcho ) {

    // Update screen first before we give the buffer away.
    // We don't do local echo ANSI strings.  Right now all of
    // our ANSI strings (and only them) are three bytes long,
    // so this is an easy way to detect them.
    if ( buf->b.dataLen < 3 ) {
      s.add( (char *)buf->data, buf->b.dataLen );
    }

  }

  if ( buf->b.dataLen ) {

    // Send the packet.  We know we had room in the socket outgoing queue so
    // this only fails if the socket is dead.

    if ( mySocket->enqueue( &buf->b ) != 0 ) {

      // Put the transmit buffer back in the pool.  The user input is lost
      // but we're not going to be running long anyway given that the socket
      // is probably closed.

      TcpBuffer::returnXmitBuf( (TcpBuffer *)buf );

    }

  }

}





void errorResolvingServer( void ) {
  s.add( "Error resolving server: " );
  s.add( serverAddrName );
  s.add( "\r\n" );
  shutdown( -1 );
}


void checkForUserExit( void ) {

  if ( bioskey(kKeyCheck) ) {
    char c = getch( );
    if ( c == 3 || c == 27 ) {
      s.add( "[Ctrl-C] or [Esc] pressed - quitting.\r\n" );
      shutdown( -1 );
    }
  }

  if ( CtrlBreakDetected ) {
    s.add( "[Ctrl-Break] pressed - quitting.\r\n" );
    shutdown( -1 );
  }

}


// Only return from here if we connected.  If there is a failure this
// function will just end the program.

void resolveAndConnect( void ) {

  s.add( "Resolving server address - press [ESC] to abort\r\n\r\n" );

  int8_t rc;

  // Resolve the name and force it to send the request for the first time.

  rc = Dns::resolve( serverAddrName, serverAddr, 1 );
  if ( rc < 0 ) errorResolvingServer( );

  while ( 1 ) {

    checkForUserExit( );

    if ( !Dns::isQueryPending( ) ) break;

    PACKET_PROCESS_SINGLE;
    Arp::driveArp( );
    Tcp::drivePackets( );
    Dns::drivePendingQuery( );

  }

  // Query is no longer pending or we bailed out of the loop.
  rc = Dns::resolve( serverAddrName, serverAddr, 0 );

  if ( rc != 0 ) errorResolvingServer( );

  sprintf( tmpBuf, "Server %s resolved to %d.%d.%d.%d\r\nConnecting to port %u...\r\n\r\n",
           serverAddrName, serverAddr[0], serverAddr[1],
           serverAddr[2], serverAddr[3], serverPort );
  s.add( tmpBuf );


  // Make the socket connection

  mySocket = TcpSocketMgr::getSocket( );
  if ( mySocket->setRecvBuffer( TCP_RECV_BUF_SIZE ) ) {
    s.add( "Ouch!  Not enough memory to run!\r\n\r\n" );
    shutdown( -1 );
  }


  rc = mySocket->connectNonBlocking( rand()%2000+2048, serverAddr, serverPort );

  if ( rc == 0 ) {

    clockTicks_t start = TIMER_GET_CURRENT( );

    while ( 1 ) {

      PACKET_PROCESS_SINGLE;
      Tcp::drivePackets( );
      Arp::driveArp( );

      if ( mySocket->isConnectComplete( ) ) return;

      checkForUserExit( );

      if ( mySocket->isClosed( ) || (Timer_diff( start, TIMER_GET_CURRENT( ) ) > TIMER_MS_TO_TICKS( ConnectTimeout )) ) {
        break;
      }

      // Sleep for 50 ms just in case we are cutting TRACE records at
      // a furious pace.
      delay(50);
    }

  }


  s.add( "Socket connection failed\r\n" );
  shutdown( -1 );

}



void sendInitialTelnetOpts( void ) {

  MyTelnetOpts.setWantRmtOn( TELOPT_ECHO );
  MyTelnetOpts.setWantRmtOn( TELOPT_SGA );

  MyTelnetOpts.setWantLclOn( TELOPT_SGA );
  MyTelnetOpts.setWantLclOn( TELOPT_TERMTYPE );
  MyTelnetOpts.setWantLclOn( TELOPT_WINDSIZE );


  // If the remote side tells us they are going to BINARY we will allow it
  // If they tell us to go to BINARY we will allow it too.
  MyTelnetOpts.setWantRmtOn( TELOPT_BIN );
  MyTelnetOpts.setWantLclOn( TELOPT_BIN );


  // Send initial Telnet options
  if ( RawOrTelnet ) {

    MyTelnetOpts.setDoOrDontPending( TELOPT_ECHO );
    MyTelnetOpts.setDoOrDontPending( TELOPT_SGA );

    // MyTelnetOpts.setDoOrDontPending( TELOPT_BIN );
    // MyTelnetOpts.setWillOrWontPending( TELOPT_BIN );

    uint8_t output[] = { TEL_IAC, TELCMD_DO, TELOPT_ECHO,
                         TEL_IAC, TELCMD_DO, TELOPT_SGA };
    //                   TEL_IAC, TELCMD_DO, TELOPT_BIN,
    //                   TEL_IAC, TELCMD_WILL, TELOPT_BIN };

    send( output, sizeof(output) );
    Tcp::drivePackets( );
  }

}



void setTelnetBinaryMode( bool binaryMode ) {

  if ( !RawOrTelnet ) return;


  // We always try to turn it on/off for both directions at the same time.
  // Assume it works and only check to see if the local side is on/off.

  if ( (MyTelnetOpts.isLclOn( TELOPT_BIN ) ? true : false) == binaryMode ) return;

  MyTelnetOpts.setDoOrDontPending( TELOPT_BIN );
  MyTelnetOpts.setWillOrWontPending( TELOPT_BIN );

  uint8_t output[6];
  output[0] = TEL_IAC;
  output[2] = TELOPT_BIN;
  output[3] = TEL_IAC;
  output[5] = TELOPT_BIN;

  if ( binaryMode == true ) {
    MyTelnetOpts.setWantRmtOn( TELOPT_BIN );
    MyTelnetOpts.setWantLclOn( TELOPT_BIN );
    output[1] = TELCMD_DO;
    output[4] = TELCMD_WILL;
  }
  else {
    MyTelnetOpts.setWantRmtOff( TELOPT_BIN );
    MyTelnetOpts.setWantLclOff( TELOPT_BIN );
    output[1] = TELCMD_DONT;
    output[4] = TELCMD_WONT;
  }

  send( output, 6 );
  Tcp::drivePackets( );
}




typedef _Packed struct {
  uint8_t x;
  uint8_t y;
  uint8_t *attr;
  char *string;
} HelpMenu_t;

HelpMenu_t HelpMenu[] = {
  { 0,  2, &scTitle,      CopyrightMsg1},
  { 2,  3, &scTitle,      CopyrightMsg2},
  { 0,  5, &scNormal,     "Commands:"},
  {10,  5, &scCommandKey, "Alt-H" },
  {16,  5, &scNormal,     "Help" },
  {27,  5, &scCommandKey, "Alt-R" },
  {33,  5, &scNormal,     "Refresh" },
  {44,  5, &scCommandKey, "Alt-X" },
  {50,  5, &scNormal,     "Exit" },
#ifdef FILEXFER
  {10,  6, &scCommandKey, "Alt-D" },
  {16,  6, &scNormal,     "Download" },
  {27,  6, &scCommandKey, "Alt-U" },
  {33,  6, &scNormal,     "Upload" },
  {44,  6, &scCommandKey, "Alt-F" },
  {50,  6, &scNormal,     "DOS Shell" },
#else
  {10,  6, &scCommandKey, "Alt-F" },
  {16,  6, &scNormal,     "DOS Shell" },
#endif
  { 0,  7, &scNormal,     "Toggles:" },
  {10,  7, &scCommandKey, "Alt-E" },
  {16,  7, &scNormal,     "Local Echo On/Off" },
  {36,  7, &scCommandKey, "Alt-W" },
  {42,  7, &scNormal,     "Wrap at right margin On/Off" },
  {10,  8, &scCommandKey, "Alt-B" },
  {16,  8, &scNormal,     "Send Backspace as Delete On/Off" },
  {50,  8, &scCommandKey, "Alt-P" },
  {56,  8, &scNormal,     "Enable Printing On/Off" },
  {10,  9, &scCommandKey, "Alt-N" },
  {16,  9, &scNormal,     "Send [Enter] as CR/NUL, CR/LF, CR or LF" },
  { 0, 11, &scNormal,     "Virtual buffer pages:    Echo:       Wrap:      Term type:" },
  { 0, 12, &scNormal,     "Print:     Send Backspace As Delete:      Send [Enter] as:"},
  { 0, 17, &scBright,     "Press a key to go back to your session ..." },
  {99,  0, 0, ""}
};

char *NewLineModes[] = {
  "CR/LF",
  "CR",
  "LF",
  "CR/NUL",
  "AUTO"
};

void doHelp( void ) {

  s.doNotUpdateRealScreen( );

  UserInputMode = UserInputMode_t::Help;

  uint16_t *start = (uint16_t far *)(s.Screen_base + 2*s.terminalCols);
  for ( int i = 0; i < 17; i++ ) {
    if ( s.isPreventSnowOn( ) & ((i & 0x1) == 0) ) { waitForCGARetraceLong( ); }
    fillUsingWord( start, (scNormal<<8|32), s.terminalCols );
    start += s.terminalCols;
  }

  s.repeatCh( 0, 1, scBorder, 205, s.terminalCols );

  int i = 0;
  while ( HelpMenu[i].x != 99 ) {
    s.myCprintf( HelpMenu[i].x, HelpMenu[i].y, *HelpMenu[i].attr, HelpMenu[i].string );
    i++;
  }

  s.myCprintf( 22, 11, scToggleStatus, "%d  ", BackScrollPages );
  s.myCprintf( 31, 11, scToggleStatus, "%s", (LocalEcho?"On":"Off") );
  s.myCprintf( 43, 11, scToggleStatus, "%s", (s.isWrapModeOn( )?"On":"Off") );
  s.myCprintf( 59, 11, scToggleStatus, "%s", TermType );
  s.myCprintf( 7,  12, scToggleStatus, "%s", (s.PrintingAllowed?"On":"Off") );
  s.myCprintf( 37, 12, scToggleStatus, "%s", (SendBsAsDel?"On":"Off") );
  s.myCprintf( 59, 12, scToggleStatus, NewLineModes[NewLineMode] );

  s.myCprintf( 0, 14, scNormal, "Tcp: Sent %lu Rcvd %lu Retrans %lu Seq/Ack errs %lu Dropped %lu",
               Tcp::Packets_Sent, Tcp::Packets_Received, Tcp::Packets_Retransmitted,
               Tcp::Packets_SeqOrAckError, Tcp::Packets_DroppedNoSpace );
  s.myCprintf( 0, 15, scNormal, "Packets: Sent: %lu Rcvd: %lu Dropped: %lu SendErrs: LowFreeBufs: %u",
           Packets_sent, Packets_received, Packets_dropped, Packets_send_errs, Buffer_lowFreeCount );

  s.repeatCh( 0, 18, scBorder, 205, s.terminalCols );

  gotoxy( 43, 17 );
}




char *HelpText[] = {
  "\ntelnet [options] <ipaddr> [port]\n\n",
  "Options:\n",
  "  -help                      Shows this help\n",
  "  -sessiontype <telnet|raw>  Force telnet mode or raw mode instead\n",
  NULL
};



static void usage( void ) {
  uint16_t i=0;
  while ( HelpText[i] != NULL ) {
    printf( HelpText[i] );
    i++;
  }
  mouse_hide();
  exit( 1 );
}



static void parseArgs( int argc, char *argv[] ) {

  bool RawOrTelnetForced = false;

  int i=1;
  for ( ; i<argc; i++ ) {

    if ( argv[i][0] != '-' ) break;

    if ( stricmp( argv[i], "-help" ) == 0 ) {
      usage( );
    }
    else if ( stricmp( argv[i], "-debug_telnet" ) == 0 ) {
      #ifndef NOTRACE
      strcpy( Trace_LogFile, "telnet.log" );
      Trace_Debugging |= 3;
      DebugTelnet = 1;
      #endif
    }
    else if ( stricmp( argv[i], "-debug_ansi" ) == 0 ) {
      #ifndef NOTRACE
      strcpy( Trace_LogFile, "telnet.log" );
      Trace_Debugging |= 3;
      DebugAnsi = 1;
      #endif
    }
    else if ( stricmp( argv[i], "-sessiontype" ) == 0 ) {
      i++;
      if ( i == argc ) {
        puts( "Must specify a session type with the -sessiontype option" );
        usage( );
      }
      if ( stricmp( argv[i], "raw" ) == 0 ) {
        RawOrTelnet = false;
        RawOrTelnetForced = true;
      }
      else if ( stricmp( argv[i], "telnet" ) == 0 ) {
        RawOrTelnet = true;
        RawOrTelnetForced = true;
      }
      else {
        puts( "Unknown session type specified on the -sessiontype option" );
        usage( );
      }
    }
    else {
      printf( "Unknown option %s\n", argv[i] );
      usage( );
    }

  }

  if ( i < argc ) {
    strncpy( serverAddrName, argv[i], SERVER_NAME_MAXLEN-1 );
    serverAddrName[SERVER_NAME_MAXLEN-1] = 0;
    i++;
  }
  else {
    printf( "Need to specify a server name to connect to.\n" );
    usage( );
  }

  if ( i < argc ) {
    serverPort = atoi( argv[i] );
    if (serverPort == 0) {
      printf( "If you specify a port it can't be this: %s\n", argv[i] );
      usage( );
    }
    if ( serverPort != 23 && (!RawOrTelnetForced) ) RawOrTelnet = false;
  }


}


void getCfgOpts( void ) {

  Utils::openCfgFile( );

  char tmp[50];
  char *printerInitChar;

  if ( Utils::getAppValue( "TELNET_VIRTBUFFER_PAGES", tmp, 10 ) == 0 ) {
    BackScrollPages = atoi( tmp );
    if ( BackScrollPages == 0 ) BackScrollPages = 1;
  }

  if ( Utils::getAppValue( "TELNET_CONNECT_TIMEOUT", tmp, 10 ) == 0 ) {
    ConnectTimeout = ((uint32_t)atoi(tmp)) * 1000ul;
    if ( ConnectTimeout == 0 ) {
      ConnectTimeout = TELNET_CONNECT_TIMEOUT;
    }
  }

  if ( Utils::getAppValue( "TELNET_AUTOWRAP", tmp, 10 ) == 0 ) {
    InitWrapMode = atoi(tmp);
    InitWrapMode = InitWrapMode != 0;
  }

  if ( Utils::getAppValue( "TELNET_PRINTER_INIT", tmp, 48 ) == 0 ) {
    printerInitChar = printerInit;
    int n = strlen(tmp);
    for(int i = 0; i < n; ++i){
      if ( *(tmp + i) == 48 && *(tmp + i + 1) == 120 && i < ( n - 3 ) ){
        i += 2;
      }
      if ( ( *(tmp + i) >= 65 && *(tmp + i) <= 70 ) || ( *(tmp + i) >= 97 && *(tmp + i) <= 102 ) || ( *(tmp + i) >= 48 && *(tmp + i) <= 57 ) ){
        if ( sscanf( tmp + i, "%hhx", printerInitChar ) == 1 ){
          printerInitChar++;
          i++;
        }
      }
    }
    *printerInitChar = 0;
  }

  if ( Utils::getAppValue( "TELNET_SENDBSASDEL", tmp, 10 ) == 0 ) {
    SendBsAsDel = atoi(tmp);
    SendBsAsDel = SendBsAsDel != 0;
  }

  if ( Utils::getAppValue( "TELNET_CGA", tmp, 10 ) == 0 ) {
    if ( atoi(tmp) >= 1 ){
      g.useMcga = false;
    }
  }

  if ( Utils::getAppValue( "TELNET_HGC", tmp, 10 ) == 0 ) {
    if ( atoi(tmp) >= 1 ){
      g.useMcga = false;
      g.useHercules = true;
      enable_hercules();
    }
  }

#ifdef USE_UTF
  s.nonPrintable = 0xa8;
  if ( Utils::getAppValue( "TELNET_UTF_DEFAULT", tmp, 10 ) == 0 ) {
    sscanf( tmp, "%x", &(s.nonPrintable) );
  }

  if ( Utils::getAppValue( "TELNET_CODEPAGE", tmp, 10 ) == 0 ) {
    s.transTableCount = Utils::getUtfValues(tmp, s.transTable, 0);
  }
  else {
    s.transTableCount = 0;
  }

  bool shouldFree;
  if ( s.transTableCount == 0 ) {
    s.transTable = defaultTransTable;
    s.transTableCount = NUMBER_OF_DEFAULT_UTF_TRANSLATIONS;
    shouldFree = false;
  }
  else {
    s.transTable = (utfTrans *)malloc( s.transTableCount * sizeof(utfTrans) );
    Utils::getUtfValues(tmp, s.transTable, 1);
    shouldFree = true;
  }
  Utils::sortUtfValues(s.transTable, &s.transTableCount);

  s.transIndex = (uint16_t*)malloc( 257 * sizeof(uint16_t) );
  //s.transLookup = (uint32_t*)malloc( s.transTableCount * sizeof(uint32_t) );
  int prevLSB;
  int currentLSB;
  bool firstTime = true;
  uint16_t i;
  int j;
  for (i = 0; i < s.transTableCount; ++i){
    currentLSB = (int)(s.transTable[i].utfChar & 0xff);
    //s.transLookup[i] = (uint32_t)((s.transTable[i].utfChar & 0xffffff00) | s.transTable[i].screenChar);
    if (firstTime || currentLSB != prevLSB){
      if (firstTime){
        firstTime = false;
        j = 0;
      }
      else{
        j = prevLSB + 1;
      }
      while (j <= currentLSB){
        s.transIndex[j++] = i;
      }
    }
    prevLSB = currentLSB;
  }
  j = prevLSB + 1;
  while (j <= 0x100){
    s.transIndex[j++] = i;
  }
  //if (shouldFree){
  //  free(s.transTable);
  //}
#endif

  if ( Utils::getAppValue( "TELNET_SEND_NEWLINE", tmp, 10 ) == 0 ) {
    if ( stricmp( tmp, "CR/LF" ) == 0 ) {
      NewLineMode = 0;
    }
    else if ( stricmp( tmp, "CR" ) == 0 ) {
      NewLineMode = 1;
    }
    else if ( stricmp( tmp, "LF" ) == 0 ) {
      NewLineMode = 2;
    }
    else if ( stricmp( tmp, "CR/NUL" ) == 0 ) {
      NewLineMode = 3;
    }
    else if ( stricmp( tmp, "AUTO" ) == 0 ) {
      NewLineMode = 4;
    }
  }

  char tmpTermType[TERMTYPE_MAXLEN];
  if ( Utils::getAppValue( "TELNET_TERMTYPE", tmpTermType, TERMTYPE_MAXLEN ) == 0 ) {
    // Uppercase is the convention
    strupr( tmpTermType );
    strcpy( TermType, tmpTermType );
  }

  Utils::getAppValue( "TELNET_COLOR_SCHEME", tmp, 10 );
  if ( stricmp( tmp, "CGA_MONO" ) == 0 ) {
    ColorScheme = 1;
  }

  Utils::closeCfgFile( );

}



static void shutdown( int rc ) {

  // Take advantage of our screen variable being a global and initialized to
  // zeros.  If the Screen_base member is set then the screen was
  // initialized
  //
  if ( s.Screen_base != NULL ) s.paint( );

  Utils::endStack( );
  mouse_hide();
  mouse_uninstall();
  if ( g.useHercules ){
    disable_hercules();
  }
  // if ( s.printerFile != NULL ) {
  //   fclose( s.printerFile );
  // }
  // if ( s.printerBuffer ) {
  //   free( s.printerBuffer );
  // }
  // if ( s.transTableCount != 0 ) {
  //   free( s.transTable );
  // }
  exit( rc );
}





// processSocket - read and process the data received from the socket
//
// Return code is the number of bytes that were left in the buffer and not
// processed.

static uint16_t processSocket( uint8_t *recvBuffer, uint16_t len ) {
  if ( mouse.xterm_any && mouse.showing ) {
    mouse_hide();
  }

  uint16_t i;
  for ( i=0; i < len; i++ ) {

    if ( StreamState == ESC_SEEN ) {

      if ( recvBuffer[i] == '[' ) {

        // Wipe out the CSI parsing variables here.  processCSISeq
        // can not do it because it must be able to pick up where it left
        // off if we had an incomplete sequence.

        for ( uint16_t j = 0; j < CSI_ARGS; j++ ) parms[j] = CSI_DEFAULT_ARG;
        parmsFound = 0;
        decPrivateControl = false;
        csiParseState = LookForPrivateControl;
        traceBufferLen = 0;

        StreamState = CSI_SEEN;
      }
      else if ( recvBuffer[i] == ']' ) {

        for ( uint16_t j = 0; j < CSI_ARGS; j++ ) parms[j] = CSI_DEFAULT_ARG;
        parmsFound = 0;
        oscParseState = LookForPs;
        clipboardType = 's';
        cutBuffer = 0;
        clipboardStore = true;

        StreamState = OSC_SEEN;

      }
      else if ( recvBuffer[i] == 'P' ) {

        if ( g.sixelReady ){
          break;
        }
        g.reset();
        StreamState = DCS_SEEN;
      }
      else {
        // Esc char was eaten - return to normal processing.

        StreamState = Normal;

        // We can handle simple one character escape sequences without
        // too much drama.
        processNonCSIEscSeq( recvBuffer[i] );
      }

    }

    else if ( StreamState == CSI_SEEN ) {

      uint16_t rc = processCSISeq( recvBuffer+i, (len-i) );

      // Bump i by the number of *additional* bytes processed
      i = i + rc - 1;

      s.updateVidBufPtr( );
    }

    else if ( StreamState == OSC_SEEN ) {

      uint16_t rc = processOSCSeq( recvBuffer+i, (len-i) );

      // Bump i by the number of *additional* bytes processed
      i = i + rc - 1;

      //s.updateVidBufPtr( );
    }
    else if ( StreamState == DCS_SEEN ) {

      uint16_t rc = g.processDCSSeq( recvBuffer+i, (len-i) );

      // Bump i by the number of *additional* bytes processed
      i = i + rc - 1;

      if ( g.done ){
        StreamState = Normal;
      }
      //s.updateVidBufPtr( );
    }
#ifdef USE_UTF
    else if ( StreamState == UTF2_SEEN ) {

      utf_chars[1] = recvBuffer[i];
      s.add( utf_chars, 2 );
      StreamState = Normal;

    }
    else if ( StreamState == UTF3_SEEN ) {

      uint16_t rc = processUTFSeq( recvBuffer+i, (len-i), 3 );

      i = i + rc - 1;

    }
    else if ( StreamState == UTF4_SEEN ) {

      uint16_t rc = processUTFSeq( recvBuffer+i, (len-i), 4 );

      i = i + rc - 1;

    }
#endif
    else if ( StreamState == IAC_SEEN ) {

      if ( MyTelnetOpts.isRmtOn( TELOPT_BIN ) && recvBuffer[i] == TEL_IAC ) {
        // Treat has a normal character.  This is ugly, but should
        // also be rare.
        s.add( (char *)(recvBuffer+i), 1 );
      }
      else {

        // It really is a telnet command ...
        int16_t rc = processTelnetCmds( recvBuffer+i, (len-i) );

        // If a telnet option is processed move 'i' forward the correct
        // number of chars.  Keep in mind that TEL_IAC has already been
        // seen, so we are passing the next character to the telnet
        // options parser.
        //
        // If a zero comes back we either did not have a full telnet command
        // in the buffer to parse or there was a socket error.  If there was
        // not a full telnet command to parse then preemptively slide the buffer
        // down to make more room for future characters from the socket, assuming
        // that we will get more input.
        //
        // The buffer slide doesn't help in our error condition, but eventually
        // we'll read the socket and figure out there is a problem.
        //
        // Remember, rc can be -1.  In that case do nothing; the socket is dead
        // and that will be detected soon enough.

        if ( rc == 0 ) {
          // Ran out of data in the buffer!
          // Move data and break the loop.
          memmove( recvBuffer, recvBuffer+i, (len-i) );
          break;
        } else if ( rc > 0 ) {
          i = i + (rc-1);
        }

      }

      StreamState = Normal;
    }


    else {

      if ( (RawOrTelnet) && recvBuffer[i] == TEL_IAC ) {
        StreamState = IAC_SEEN;
      }

      else if ( recvBuffer[i] == 27 ) {
        s.suppressOverhang( );
        StreamState = ESC_SEEN;
      }

#ifdef USE_UTF
      else if ( recvBuffer[i] >= 0xf0 ) {
        StreamState = UTF4_SEEN;
        utf_chars[0] = recvBuffer[i];
        utf_chars_seen = 1;
      }

      else if ( recvBuffer[i] >= 0xe0 ) {
        StreamState = UTF3_SEEN;
        utf_chars[0] = recvBuffer[i];
        utf_chars_seen = 1;
      }

      else if ( recvBuffer[i] >= 0x80 ) {
        StreamState = UTF2_SEEN;
        utf_chars[0] = recvBuffer[i];
      }
#endif
      else {

        // Not a telnet or an ESC code.  Do screen handling here.

        // This is a lot of overhead for just one character.  Scan
        // ahead to see if we can do a few characters to improve
        // performance.
        //


        uint16_t bufLen = 1;
        while ( (i+bufLen < len) && (recvBuffer[i+bufLen] != 27) && (recvBuffer[i+bufLen] < 0x7f) && (recvBuffer[i+bufLen] != TEL_IAC) ) bufLen++;
        s.add( (char *)(recvBuffer+i), bufLen );
        i = i + bufLen - 1;

      }

    }

  }  // end for

  if ( mouse.xterm_any && mouse.should_show ) {
    mouse_show();
  }

  if ( g.sixelReady ) {
    if ( g.sixelInvalid == 0 ){
      mouse_hide();
      s.usingGraphics = true;
      g.showImage();
    }
    else {
      sprintf( tmpBuf, "\r\nSixel error: %d c%d %dx%d %d\r\n", g.sixelInvalid, g.sixelColors, g.sixelPh, g.sixelPv, g.videoMode );
      s.add(tmpBuf);
    }
    g.sixelReady = false;
  }

  return (len - i);
}






// Telnet negotiation from Pg 403 of TCP/IP Illustrated Vol 1
//
// Sender    Receiver
// Will      Do        Sender wants, receiver agrees
// Will      Dont      Sender wants, receiver says no
// Do        Will      Sender wants other side to do it, receiver will
// Do        Wont      Sender wants other side to do it, receiver wont
// Wont      Dont      Sender says no way, receiver must agree
// Dont      Wont      Sender says dont do it, receiver must agree
//
// Page 1451 in TCP/IP Guide is good too.


// processTelnetCmds
//
// By the time we get here we have seen TEL_IAC.
//
// Process the first Telnet command, and then enter a loop to process
// any others that might be in the input.  They often come in groups.
// Try to buildup a single packet with out responses to avoid using
// up all of our outgoing packets with small responses.
//
// When we do not find any more options return the number of input
// bytes that we consumed.  Also push out our response packet, and
// ensure that it actually goes out.  (We will sit and wait for all
// of the bytes to be sent.)
//
// Return codes: n is the number of input bytes consumed
//               0 if the input buffer was incomplete (try again later)


uint8_t telnetOptionsOutput[ 100 ];

int16_t processTelnetCmds( uint8_t *cmdStr, uint8_t cmdSize ) {

  int16_t inputBytesConsumed = 0;
  uint16_t outputBufLen = 0;;

  uint16_t localOutputBufLen;
  int16_t localInputBytesConsumed = processSingleTelnetCmd( cmdStr, cmdSize, telnetOptionsOutput, &localOutputBufLen );

  if (localInputBytesConsumed == 0) {
    // Incomplete input to parse a full Telnet option - return to try again later.
    return 0;
  }


  outputBufLen = localOutputBufLen;
  inputBytesConsumed = localInputBytesConsumed;
  cmdStr += localInputBytesConsumed;
  cmdSize -= localInputBytesConsumed;


  // Ensure a minimum of 50 chars are available for output from processSingleTelnetCmd.
  // It does not do overflow checking, so it had better fit.

  while ( ((100 - outputBufLen) > 50 ) && (cmdSize > 1) && (*cmdStr == TEL_IAC) ) {

    // Another Telnet option!
    //
    // Ensure that if we are in Telnet BINARY mode that we handle two TEL_IACs consecutive correctly.

    if ( MyTelnetOpts.isRmtOn( TELOPT_BIN ) ) {
      if ( *(cmdStr+1) == TEL_IAC ) {
        // Not ours!  Let our caller handle it.
        break;
      }
    }

    localInputBytesConsumed = processSingleTelnetCmd( cmdStr+1, cmdSize-1, telnetOptionsOutput+outputBufLen, &localOutputBufLen );

    if ( localInputBytesConsumed == 0 ) {
      // Incomplete input to parse the option; skip this for now.
      break;
    }

    outputBufLen += localOutputBufLen;

    // Need to skip an extra byte for the initial TEL_IAC

    inputBytesConsumed += localInputBytesConsumed + 1;
    cmdStr += localInputBytesConsumed + 1;
    cmdSize -= localInputBytesConsumed + 1;

  }


  if ( DebugTelnet ) {
    TRACE(( "Consumed %d bytes of telnet options bytes, Sending %d bytes of response data\n", inputBytesConsumed, outputBufLen ));
  }

  send( telnetOptionsOutput, outputBufLen );

  return inputBytesConsumed;
}





// Process one telnet command.  The TEL_IAC character is already consumed
// before this is called so we are dealing with the second character in
// the sequence.
//
// The caller provides the output buffer, and it must have enough available
// space in it.  At the moment we set that to 50 chars which is far more
// than we will ever need for one telnet option response.  That keeps us from
// having to check each time we add a byte to the output.

// Return codes: n is the number of input bytes consumed
//               0 if the input buffer was incomplete (try again later)
//
// The outputBufLen parm is used a secondary return code.

const char DoOrDontPendingErrMsg[] = "Was waiting for a reply so no response sent\n";

static int16_t processSingleTelnetCmd( uint8_t *cmdStr, uint8_t inputBytes, uint8_t *outputBuf, uint16_t *outputBufLen ) {

  uint8_t localOutputBufLen = 0;

  // Set return parameter to something sane before getting involved.
  *outputBufLen = 0;

  // Not enough input.
  if ( inputBytes < 1 ) return 0;

  char debugMsg[120];
  uint16_t debugMsgLen = 0;

  // Return code is how many bytes to remove from stream.
  uint16_t inputBytesConsumed = 1;

  switch ( cmdStr[0] ) {


    case TELCMD_WILL: {

      if ( inputBytes < 2 ) return 0;

      inputBytesConsumed = 2;

      uint8_t respCmd;

      uint8_t cmd = cmdStr[1];      // Actual command sent from server
      uint8_t cmdTableIndex = cmd;  // Index into telnet options to look at

      // Protect TelnetOpts class from high numbered options.
      // If it is too high point at a bogus table entry that has everything
      // turned off.
      if ( cmdTableIndex >= TEL_OPTIONS ) cmdTableIndex = TEL_OPTIONS-1;

      if ( DebugTelnet ) {
        debugMsgLen = sprintf( debugMsg, "Received WILL %u, ", cmd );
      }

      if ( MyTelnetOpts.isWantRmtOn( cmdTableIndex ) ) {
        respCmd = TELCMD_DO;
        MyTelnetOpts.setRmtOn( cmdTableIndex );
      }
      else {
        respCmd = TELCMD_DONT;
        MyTelnetOpts.setRmtOff( cmdTableIndex );
      }

      if ( MyTelnetOpts.isDoOrDontPending( cmdTableIndex ) ) {
        MyTelnetOpts.clrDoOrDontPending( cmdTableIndex );
        if ( DebugTelnet ) {
          debugMsgLen += sprintf( debugMsg+debugMsgLen, DoOrDontPendingErrMsg );
        }
      }
      else {
        outputBuf[0] = TEL_IAC;
        outputBuf[1] = respCmd;
        outputBuf[2] = cmd;
        localOutputBufLen = 3;
        if ( DebugTelnet ) {
          debugMsgLen += sprintf( debugMsg+debugMsgLen, "Sent %s\n", (respCmd==TELCMD_DO?"DO":"DONT") );
        }
      }

      break;

    }


    case TELCMD_WONT: {

      if ( inputBytes < 2 ) return 0;

      inputBytesConsumed = 2;

      uint8_t cmd = cmdStr[1];      // Actual command sent from server
      uint8_t cmdTableIndex = cmd;  // Index into telnet options to look at

      // Protect TelnetOpts class from high numbered options.
      // If it is too high point at a bogus table entry that has everything
      // turned off.
      if ( cmdTableIndex >= TEL_OPTIONS ) cmdTableIndex = TEL_OPTIONS-1;

      if ( DebugTelnet ) {
        debugMsgLen = sprintf( debugMsg, "Received WONT %u, ", cmd );
      }

      // Our only valid response is DONT

      MyTelnetOpts.setRmtOff( cmdTableIndex );

      if ( MyTelnetOpts.isDoOrDontPending( cmdTableIndex ) ) {
        MyTelnetOpts.clrDoOrDontPending( cmdTableIndex );
        if ( DebugTelnet ) {
          debugMsgLen += sprintf( debugMsg+debugMsgLen, DoOrDontPendingErrMsg );
        }
      }
      else {
        outputBuf[0] = TEL_IAC;
        outputBuf[1] = TELCMD_DONT;
        outputBuf[2] = cmd;
        localOutputBufLen = 3;
        if ( DebugTelnet ) {
          debugMsgLen += sprintf( debugMsg+debugMsgLen, "Sent DONT\n" );
        }
      }

      break;
    }


    case TELCMD_DO: {

      if ( inputBytes < 2 ) return 0;

      inputBytesConsumed = 2;

      uint8_t respCmd;

      uint8_t cmd = cmdStr[1];      // Actual command sent from server
      uint8_t cmdTableIndex = cmd;  // Index into telnet options to look at

      // Protect TelnetOpts class from high numbered options.
      // If it is too high point at a bogus table entry that has everything
      // turned off.
      if ( cmdTableIndex >= TEL_OPTIONS ) cmdTableIndex = TEL_OPTIONS-1;

      if ( DebugTelnet ) {
        debugMsgLen = sprintf( debugMsg, "Received DO   %u, ", cmd );
      }

      if ( MyTelnetOpts.isWantLclOn( cmdTableIndex ) ) {
        respCmd = TELCMD_WILL;
        MyTelnetOpts.setLclOn( cmdTableIndex );
      }
      else {
        respCmd = TELCMD_WONT;
        MyTelnetOpts.setLclOff( cmdTableIndex );
      }

      if ( MyTelnetOpts.isWillOrWontPending( cmdTableIndex ) ) {
        MyTelnetOpts.clrWillOrWontPending( cmdTableIndex );
        if ( DebugTelnet ) {
          debugMsgLen += sprintf( debugMsg+debugMsgLen, DoOrDontPendingErrMsg );
        }
      }
      else {
        outputBuf[0] = TEL_IAC;
        outputBuf[1] = respCmd;
        outputBuf[2] = cmd;
        localOutputBufLen = 3;
        if ( DebugTelnet ) {
          debugMsgLen += sprintf( debugMsg+debugMsgLen, "Sent %s\n", (respCmd==TELCMD_WILL?"WILL":"WONT") );
        }
      }

      if ( cmd == TELOPT_WINDSIZE && respCmd == TELCMD_WILL ) {
        outputBuf[3] = TEL_IAC;
        outputBuf[4] = TELCMD_SUBOPT_BEGIN;
        outputBuf[5] = TELOPT_WINDSIZE;
        outputBuf[6] = 0;
        outputBuf[7] = s.terminalCols;
        outputBuf[8] = 0;
        outputBuf[9] = s.terminalLines;
        outputBuf[10] = TEL_IAC;
        outputBuf[11] = TELCMD_SUBOPT_END;
        localOutputBufLen = 12;
      }


      break;

    }


    case TELCMD_DONT: {

      if ( inputBytes < 2 ) return 0;

      inputBytesConsumed = 2;

      uint8_t cmd = cmdStr[1];      // Actual command sent from server
      uint8_t cmdTableIndex = cmd;  // Index into telnet options to look at

      // Protect TelnetOpts class from high numbered options.
      // If it is too high point at a bogus table entry that has everything
      // turned off.
      if ( cmdTableIndex >= TEL_OPTIONS ) cmdTableIndex = TEL_OPTIONS-1;

      if ( DebugTelnet ) {
        debugMsgLen = sprintf( debugMsg, "Received DONT %u, ", cmd );
      }

      // Our only valid response is WONT

      MyTelnetOpts.setLclOff( cmdTableIndex );

      if ( MyTelnetOpts.isWillOrWontPending( cmdTableIndex ) ) {
        MyTelnetOpts.clrWillOrWontPending( cmdTableIndex );
        if ( DebugTelnet ) {
          debugMsgLen += sprintf( debugMsg+debugMsgLen, DoOrDontPendingErrMsg );
        }
      }
      else {
        outputBuf[0] = TEL_IAC;
        outputBuf[1] = TELCMD_WONT;
        outputBuf[2] = cmd;
        localOutputBufLen = 3;
        if ( DebugTelnet ) {
          debugMsgLen += sprintf( debugMsg+debugMsgLen, "Sent WONT\n" );
        }
      }

      break;
    }




    case TELCMD_SUBOPT_BEGIN: { // Suboption begin

      // First thing to do is to find TELCMD_SUBOPT_END

      uint16_t suboptEndIndex = 0;
      for ( uint16_t i=1; i < inputBytes-1; i++ ) {
        if ( cmdStr[i] == TEL_IAC && cmdStr[i+1] == TELCMD_SUBOPT_END ) {
          inputBytesConsumed = i + 2;
          suboptEndIndex = i;
        }
      }

      if ( suboptEndIndex < 3 ) return 0;

      if ( (suboptEndIndex == 3) && (cmdStr[1] == TELOPT_TERMTYPE) ) {

        if ( cmdStr[2] == 1 && cmdStr[3] == TEL_IAC && cmdStr[4] == TELCMD_SUBOPT_END ) {
          outputBuf[0] = TEL_IAC;
          outputBuf[1] = TELCMD_SUBOPT_BEGIN;
          outputBuf[2] = TELOPT_TERMTYPE;
          outputBuf[3] = 0;

          localOutputBufLen = 4;

          char *tmp = TermType;
          while (*tmp) outputBuf[localOutputBufLen++] = *tmp++;

          outputBuf[localOutputBufLen++] = TEL_IAC;
          outputBuf[localOutputBufLen++] = TELCMD_SUBOPT_END;
          if ( DebugTelnet ) {
            debugMsgLen = sprintf( debugMsg, "Sent termtype %s\n", TermType );
          }
        }

      }
      else {
        if ( DebugTelnet ) {
          debugMsgLen += sprintf( debugMsg+debugMsgLen, "Unknown SUBOPT: %u\n", cmdStr[2] );
        }
      }

      break;

    }



    case TELCMD_NOP:   // Nop
    case TELCMD_DM:    // Data Mark
    case TELCMD_BRK:   // Break (break or attention key hit)
    case TELCMD_IP:    // Interrupt process
    case TELCMD_AO:    // Abort output
    {
      if ( DebugTelnet ) {
        debugMsgLen = sprintf( debugMsg, "Telnet: Ignored command: %u\n", cmdStr[0] );
      }
      break;
    }

    case TELCMD_AYT: { // Are you there?
      // Send a null command back - that should be sufficent
      outputBuf[0] = TEL_IAC;
      outputBuf[1] = TELCMD_NOP;
      localOutputBufLen = 2;
      break;
    }

    default: {
      if ( DebugTelnet ) {
        debugMsgLen = sprintf( debugMsg, "Telnet: Unprocessed Command: %u\n", cmdStr[0] );
      }
    }

  }

  if ( DebugTelnet ) {
    TRACE(( "%s", debugMsg ));
  }


  *outputBufLen = localOutputBufLen;
  return inputBytesConsumed;

}

#ifdef USE_UTF
static uint16_t processUTFSeq( uint8_t *buffer, uint16_t len, uint16_t size ) {
  uint16_t bytesProcessed = 0;
  while ( bytesProcessed < len && utf_chars_seen < size ) {
    utf_chars[utf_chars_seen] = buffer[bytesProcessed];
    utf_chars_seen++;
    bytesProcessed++;
  }
  if ( utf_chars_seen == size ) {
    s.add( utf_chars, size );
    StreamState = Normal;
  }
  return bytesProcessed;
}
#endif
// CSI [p] c             0 or 1 parms
// CSI [p] ; [p] c       2 parms


// processCSISeq
//
// Process a single CSI sequence.  This gets called when we are in state
// CSI_SEEN, which means that we have seen ESC [.
//
// If we run out of bytes before seeing a command we will pick up
// where we left off.  The parms and trace buffer are all globals
// and will be preserved across calls.
//
// This function returns the number of bytes consumed.  It is guaranteed
// to always be at least 1.
//
// Do not call with len = 0

static uint16_t processCSISeq( uint8_t *buffer, uint16_t len ) {



  // Used only for debugging/tracing
  //
  uint16_t start_cursor_x = s.cursor_x;
  uint16_t start_cursor_y = s.cursor_y;


  // Number of bytes consumed by this processing.
  uint16_t bytesProcessed = 0;

  uint8_t commandLetter = 0;

  uint16_t i = 0;


  // Ensure that we only set this flag if it is the first character right after the CSI.

  if ( csiParseState == LookForPrivateControl ) {

    if ( buffer[i] == '?' ) {
      decPrivateControl = true;
      i++;
    }

    // Whether we find the decPrivateControl char or not, the next sequence is this.
    csiParseState = NoParmsFound;

  }




  while ( i < len ) {

    uint8_t c = buffer[i];

    // Used only for debugging/tracing these ANSI sequences
    traceBuffer[traceBufferLen++] = c;


    // Is this a numeric parm?
    if ( isdigit( c ) ) {

      if ( parmsFound < CSI_ARGS ) {

        // Ok, we have room for another parameter.

        // If this is the first character of the new parameter initialize
        // the parameter to zero.  (Parameters are always positive integers.)

        if ( parms[ parmsFound ] == CSI_DEFAULT_ARG ) parms[ parmsFound ] = 0;

        parms[ parmsFound ] = parms[ parmsFound ] * 10 + c - '0';

      }

      csiParseState = ParmsFound;

    }
    else if ( c == ';' ) {

      // Parms found never gets past 16 ... After that we parse them but
      // throw them away.

      if ( parmsFound < CSI_ARGS ) parmsFound++;
      csiParseState = ParmsFound;

    }
    else {

        if ( csiParseState == ParmsFound && ( parmsFound < CSI_ARGS ) ) parmsFound++;

        commandLetter = c;
        i++;
        break;
    }

    i++;

  } // end while

  bytesProcessed = i;

  // Run out of bytes?
  if ( commandLetter == 0 ) return bytesProcessed;

  traceBuffer[traceBufferLen] = 0;
  traceBufferLen = 0;

  //TRACE(( "Ansi: Found: %d  Parms: %d %d %d %d %d %d\n", parmsFound,
  //        parms[0], parms[1], parms[2], parms[3], parms[4], parms[5] ));

  if ( decPrivateControl == true ) {
    processDecPrivateControl( commandLetter );
  }
  else {
    processAnsiCommand( commandLetter );
  }

  //if ( DebugAnsi ) {
  //  TRACE(( "Ansi: Old cur: (%02d,%02d) New cur: (%02d,%02d) Attr: %04x Cmd: %s\n",
  //          start_cursor_x, start_cursor_y, s.cursor_x, s.cursor_y, s.curAttr, traceBuffer ));
  //}

  // Set this here instead in the routine that called us.
  // Because we remember state across calls, the routine that called us
  // never knows when to set StreamState back to normal.
  StreamState = Normal;

  return bytesProcessed;
}

static uint16_t processOSCSeq( uint8_t *buffer, uint16_t len ) {
  uint16_t bytesProcessed = 0;

  uint8_t commandLetter = 0;

  uint16_t i = 0;
  uint16_t j = 0;
  while ( i < len ) {

    uint8_t c = buffer[i];

    if ( c == 7 ) {
      commandLetter = c;
      i++;
      break;
    }
    if ( c == 0x1b ) {
      commandLetter = c;
      break;
    }
    // else if ( c == ';' ) {
    //   if ( parmsFound < CSI_ARGS ) parmsFound++;
    //   if ( oscParseState == LookForPs ) {
    //     oscParseState = LookForClipboardCode;
    //   }
    //   else if ( oscParseState == LookForClipboardCode ) {
    //     oscParseState = LookForData;
    //   }
    // }
    // else if ( oscParseState == LookForPs ) {
    //   if ( isdigit( c ) ) {
    //     if ( parms[ parmsFound ] == CSI_DEFAULT_ARG ) parms[ parmsFound ] = 0;

    //     parms[ parmsFound ] = parms[ parmsFound ] * 10 + c - '0';
    //   }
    // }
    // else if ( oscParseState == LookForClipboardCode ) {
    //   if ( isdigit( c ) ) {
    //     cutBuffer = c - '0';
    //   }
    //   else {
    //     clipboardType = c;
    //   }
    // }
    // else if ( oscParseState == LookForData ) {
    //   if ( c == '?' ) {
    //     clipboardStore = false;
    //   }
    //   incomingClipboard[j++] = c;
    // }
    i++;
  }

  bytesProcessed = i;

  if ( commandLetter == 0 ) return bytesProcessed;

  StreamState = Normal;

  // if ( clipboardStore ) {
  //   if (j == 0) { // clear the selection
  //     s.selection.buf[0] = 0;
  //   }
  //   else {
  //     incomingClipboard[j++] = 0;
  //     base64_decode(incomingClipboard, s.selection.buf, MAX_SELECTION_SIZE);
  //   }
  // }
  // else {
  //   needToSendSelection = true;
  // }

  return bytesProcessed;
}


// This can be inline because it is just called from one place.  It saves
// some bytes this way.

void processAnsiCommand( uint8_t commandLetter ) {

  switch ( commandLetter ) {

    // ICH - Insert Character
    //   Use normal character attribute; cursor does not move.  No effect outside
    //   of scroll region.

    case '@': { // ICH - Insert Character
      if ( (s.cursor_y >= s.scrollRegion_top) && (s.cursor_y <= s.scrollRegion_bottom) ) {
        if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
        s.insChars( parms[0] );
      }
      break;
    }


    // CUU - Cursor Up
    //   Screen is not scrolled if the cursor is already on the top line.
    //   Column stays the same.  Default parm is 1.
    //   If origin mode is on this does not leave the defined scroll region

    case 'A': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.adjustVertical( 0 - parms[0] );
      break;
    }


    // CUD - Cursor Down
    //   Screen is not scrolled if the cursor is already on the top line.
    //   Column stays the same.  Default parm is 1.
    //   If origin mode is on this does not leave the defined scroll region

    case 'e':   // VPR -
    case 'B': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.adjustVertical( parms[0] );
      break;
    }


    // CUF - Cursor Forward
    //   Cursor stops at the right border.  Row stays the same.  Default parm is 1.

    case 'a':   // HPR -
    case 'C': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.setHorizontal( s.cursor_x + parms[0] );
      break;
    }


    // CUB - Cursor Back
    //   Cursor stops at the left border.  Row stays the same.  Default parm is 1.

    case 'D': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.setHorizontal( s.cursor_x - parms[0] );
      break;
    }


    // CNL - Cursor Next Line
    //   Move the first column of the line n lines down.  Default parm is 1.
    //   Putty does not scroll the screen if it is at the bottom.

    case 'E': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.adjustVertical( parms[0] );
      s.cursor_x = 0;
      break;
    }


    // CPL - Cursor Previous Line
    //   Move the first column of the line n lines up.  Default parm is 1.
    //   Putty does not scroll the screen if it is at the bottom.

    case 'F': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.adjustVertical( 0 - parms[0] );
      s.cursor_x=0;
      break;
    }


    // CHA - Cursor Horizontal Absolute
    //   Move to column n.  Row is not changed.  Default parm is 1.

    case '`':   // HPA - Set Horizontal Position
    case 'G': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.setHorizontal( parms[0] - 1 );
      break;
    }


    // VPA - Vertical Position Absolute
    //   Move to row n.  Column is not changed.  Default parm is 1.

    case 'd': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      s.setVertical( parms[0] - 1 );
      break;
    }


    // CUP - Cursor Position
    // HVP - Horizontal and Vertial Position
    //   Move to row parm1, column parm2.  (Values are on based).  Default parms are 1.

    case 'f':
    case 'H': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      if ( parms[1] == CSI_DEFAULT_ARG ) parms[1] = 1;
      s.setVertical( parms[0] - 1 );
      s.setHorizontal( parms[1] - 1 );
      break;
    }


    // CHT - Cursor Horizontal Forward Tabulation
    //   Move the active position n tabs forward
    //   Putty does not seem to honor this

    case 'I': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      for ( uint16_t i=0; i < parms[0]; i++ ) {
        int16_t newCursor_x = (s.cursor_x + 8) & 0xF8;
        if ( newCursor_x < s.terminalCols ) s.cursor_x = newCursor_x;
      }
      break;
    }


    // CBT - Cursor Backward Tabulation
    //   Move the active position n tabs backward

    case 'Z': { // Backtab

      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      for ( uint16_t i=0; i < parms[0]; i++ ) {

        int16_t newCursor_x;
        if ( ((s.cursor_x & 0xF8) == s.cursor_x) && (s.cursor_x>0) ) {
          // Already at a tab stop boundary, go back eight
          newCursor_x = s.cursor_x - 8;
        }
        else {
          // Not on a tab stop boundary - just round down
          newCursor_x = s.cursor_x & 0xF8;
        }

        if ( newCursor_x >= 0 ) s.cursor_x = newCursor_x;
      }
      break;
    }


    // ED - Erase Data
    //   parm = 0: clear from cursor to end of screen (default is missing parm)
    //   parm = 1: clear from cursor to beginning of screen
    //   parm = 2: clear entire screen.  (And home cursor on DOS machines?)

    case 'J': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 0;
      mouse_hide();
      mouse.should_show = false;
      s.highlightSelection(false);
      switch ( parms[0] ) {
        case 0: {
          s.clear( s.cursor_x, s.cursor_y, s.terminalCols-1, s.terminalLines-1 );
          break;
        }
        case 1: {
          s.clear( 0, 0, s.cursor_x, s.cursor_y );
          break;
        }
        case 2: {
          s.clear( 0, 0, s.terminalCols-1, s.terminalLines-1 );

          // Strict ANSI behavior is to clear the screen, which is what Putty
          // and XTERM do.  DOS ANSI.SYS also homes the cursor.  This is going
          // to need a chicken switch.
          if ( Strict_DOS_Mode ) {
            s.cursor_x = 0;
            s.cursor_y = 0;
          }
          break;
        }
      }
      break;
    }


    // EL - Erase in Line
    //   parm = 0: clear from cursor to end of line (default is missing parm)
    //   parm = 1: clear from cursor to beginning of line
    //   parm = 2: clear entire line, no cursor change.

    case 'K': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 0;
      switch ( parms[0] ) {
        case 0: {
          s.clear( s.cursor_x, s.cursor_y, s.terminalCols-1, s.cursor_y );
          break;
        }
        case 1: {
          s.clear( 0, s.cursor_y, s.cursor_x, s.cursor_y );
          break;
        }
        case 2: {
          s.clear( 0, s.cursor_y, s.terminalCols-1, s.cursor_y );
          break;
        }
      }
      break;
    }


    // IL - Insert Lines
    //   Insert an open line at the current cursor position and scroll the rest down.
    //   Newly inserted lines have the current attribute

    case 'L': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      for ( uint16_t i=0; i < parms[0]; i++ ) s.insLine( s.cursor_y );
      break;
    }


    // DL - Delete Lines
    //   Delete at the current cursor position and scroll the rest up.

    case 'M': { // DL - Delete Lines at current cursor position
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      for ( uint16_t i=0; i < parms[0]; i++ ) s.delLine( s.cursor_y );
      break;
    }


    // SU - Scroll Up/Pan Up
    //   Page is scrolled up by n lines.  New lines added at the bottom.  Default is 1.
    //   Needs to respect the scroll window

    case 'S': { // SU/INDN - Scroll screen up without changing cursor pos
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      for ( uint16_t i=0; i < parms[0]; i++ ) s.delLine( s.scrollRegion_top );
      break;
    }


    // SD - Scroll Down/Pan Down
    //   Scroll area is scrolled down by n lines.  New lines are added at the top.  Default is 1.

    case 'T': { // RIN - Scroll screen down without changing cursor pos
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;
      for ( uint16_t i=0; i < parms[0]; i++ ) s.insLine( s.scrollRegion_top );
      break;
    }


    // SGR - Select Graphic Rendition
    //   No parm = reset/normal

    case 'm': {

      if ( parmsFound == 0 ) {
        parmsFound = 1;
        parms[0] = 0;
      }

      for ( uint16_t p=0; p < parmsFound; p++ ) {
        if ( parms[p] >= 30 && parms[p] < 40 ) {
          fg = fgColorMap[ parms[p] - 30 ];
        }
        else if ( parms[p] >= 40 ) {
          bg = bgColorMap[ parms[p] - 40 ];
        }
        else {
          switch ( parms[p] ) {

            case 0: {
              reverse = bold = blink = bg = 0;
              underline = false;
              fg = 7;
              if (g.useMcga){
                setCursorMcga( );
              } else {
                setCursor( );
              }
              break;
            }

            case  1: { bold = 1; break; }          // Bold
            case  2: { bold = 0; break; }          // Faint
            case  3: { break; }                    // Italic
            case  4: { underline = true; break; }  // Underline
            case  5: { blink = 1; break; }         // Slow blink
            case  6: { blink = 1; break; }         // Fast blink
            case  7: { reverse = 1; break; }       // Reverse
            case  8: { break; }                    // Conceal
            case 21: { underline = true; break; }  // Double underline
            case 22: { bold = 0; break; }          // Normal intensity
            case 24: { underline = false; break; } // No underline
            case 25: { blink = 0; break; }         // Blink off
            case 27: { reverse = 0; break; }       // Reverse off
            case 28: { break; }                    // Conceal off
          }
        }
      } // end for

      uint8_t newAttr;
      if ( ! reverse ) {
        newAttr = (blink<<7) | (bg<<4) | (bold<<3) | fg;
      }
      else {
        newAttr = (blink<<7) | (fg<<4) | (bold<<3) | bg;
      }

      if ( s.isColorCard( ) && underline ) newAttr = (blink<<7) | (bg<<4) | (bold<<3) | 0x01;
      s.curAttr = newAttr;
      break;
    }



    // DA - Device Attributes
    //   Response means no options

    case 'c': {
      strcpy( tmpBuf, "\033[?1;0c" );
      send( (uint8_t *)tmpBuf, 7 );
      break;
    }



    // DSR - Device Status Report   ( CSI 5 n )
    // CPR - Cursor Position Report ( CSI 6 n )

    case 'n': {

      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 0;

      switch( parms[0] ) {

        case 5: {

          strcpy( tmpBuf, "\033[0n" );
          send( (uint8_t *)tmpBuf, 4 );
          break;

        }

        case 6: {

          int16_t tmpY = s.cursor_y + 1;

          // If origin mode is on adjust the row reporting.
          if ( s.originMode == true ) s.cursor_y = s.cursor_y - s.scrollRegion_top;

          int rcBytes = sprintf( tmpBuf, "\033[%d;%dR", tmpY, (s.cursor_x+1) );
          send( (uint8_t *)tmpBuf, rcBytes );
          break;

        }
      }
      break;
    }

    case 'b': { // REP
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;

      if ( parms[0] > s.terminalCols ) {
        TRACE_WARN(( "Ansi: REP Command: parm (%u) > s.terminalCols\n", parms[0] ));
        parms[0] = s.terminalCols;
      }

      memset( tmpBuf, s.lastChar, parms[0] );

      s.add( (char *)tmpBuf, parms[0] );

      break;
    }


    // DCH - Delete Character
    //   Remaining chars slide to the left.  Default is one.

    case 'P': {
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;

      s.delChars( parms[0] );
      break;
    }

    case 'X': { // ECH - Erase Character
      if ( parms[0] == CSI_DEFAULT_ARG ) parms[0] = 1;

      s.eraseChars( parms[0] );
      break;
    }


    // SCP - Save Cursor Position

    case 's': {
      saved_cursor_x = s.cursor_x;
      saved_cursor_y = s.cursor_y;
      break;
    }


    // RCP - Restore Cursor Position

    case 'u': {
      s.cursor_x = saved_cursor_x;
      s.cursor_y = saved_cursor_y;
      break;
    }


    case 'r': { // Set scroll window

      if ( parms[0] == CSI_DEFAULT_ARG ) {
        parms[0] = 1;
        parms[1] = s.terminalLines;
      }

      // Fixme - sanity check input

      s.scrollRegion_top = parms[0] - 1;
      s.scrollRegion_bottom = parms[1] - 1;

      // The host sends these as 1 based; we use 0 for a base.

      s.setHorizontal( 0 );
      s.setVertical( 0 );

      break;

    }

    // case 'h': ANSI set options - not implemented
    // case 'l': ANSI reset options - not implemented

    case 'i': {
      if ( s.PrintingAllowed ){
        if ( parms[0] == 5 ){
          s.AutoPrint = 1;
          // if ( s.printerFile == NULL ) {
          //   s.printerFile = fopen( "dump.prn", "wb" );
          //   if ( s.printerFile == NULL ) {
          //     TRACE_WARN(( "Download: Error opening dump.prn, %s\n", strerror(errno) ));
          //     shutdown(-1);
          //   }
          // }
        }
        else if ( parms[0] == 4 ){
          s.AutoPrint = 0;
        }
      }
    }
    default: {
      TRACE_WARN(( "Ansi: Unknown cmd: %c %s\n", commandLetter, traceBuffer ));
      break;
    }


  }

}


// processDecPrivateControl
//
// processes CSI sequences that set modes for the terminal emulation.
// Only a subset of common sequences are handled.
//
// DECOM        6  Origin Mode: Cursor is not allowed out of the margins
// DECAWM       7  AutoWrap Mode
//
// DECTCEM     25  Text Cursor Enable Mode
// DECBKM      67  Backarrow Key Mode
// DECCAPSLK  108  Num lock mode
// DECCAPSLK  109  Caps lock mode
//
// There might be a few on a command line, so loop through them all.

void processDecPrivateControl( uint8_t commandLetter ) {

  switch ( commandLetter ) {

    case 'h': { // Set

      for ( uint16_t i = 0; i < parmsFound; i++ ) {

        if ( parms[i] == 6 ) {
          s.originMode = true;
        } else if ( parms[i] == 7 ) {
          s.autoWrap = true;
        } else if ( parms[i] == 25 ) {
          if (g.useMcga){
            setCursorMcga( );
          } else {
            setCursor( );
          }
        } else if ( parms[i] == 1000 ) {
          mouse.xterm_normal = true;
        } else if ( parms[i] == 1002 ) {
          mouse.xterm_motion = true;
        } else if ( parms[i] == 1006 ) {
          mouse.xterm_sgr = true;
        } else if ( parms[i] == 2004 ) {
          mouse.bracketed_paste = true;
        }
        if ( mouse.xterm_normal || mouse.xterm_sgr || mouse.xterm_motion ) {
          mouse.xterm_any = true;
        }
        else {
          mouse.xterm_any = false;
          mouse.should_show = false;
          mouse_hide();
        }

      }
      break;
    }

    case 'l': { // Reset

      for ( uint16_t i = 0; i < parmsFound; i++ ) {

        if ( parms[i] == 6 ) {
          s.originMode = false;
        } else if ( parms[i] == 7 ) {
          s.autoWrap = false;
        } else if ( parms[i] == 25 ) {
          hideCursor( );
        } else if ( parms[i] == 1000 ) {
          mouse.xterm_normal = false;
        } else if ( parms[i] == 1002 ) {
          mouse.xterm_motion = false;
        } else if ( parms[i] == 1006 ) {
          mouse.xterm_sgr = false;
        } else if ( parms[i] == 2004 ) {
          mouse.bracketed_paste = false;
        }
        if ( mouse.xterm_normal || mouse.xterm_sgr || mouse.xterm_motion ) {
          mouse.xterm_any = true;
        }
        else {
          mouse.xterm_any = false;
          mouse.should_show = false;
          mouse_hide();
        }
      }

      break;
    }

  } // end switch

}


// These are single character sequences.  The best documentation I have is at
// http://man7.org/linux/man-pages/man4/console_codes.4.html .
//
// If a character is not supported it is effectively eaten without any side
// effects.

static void processNonCSIEscSeq( uint8_t ch ) {

  if ( ch == '7' ) {

    // DECSC: Save cursor
    //
    // Specifically, save position, attributes from SGR, wrap flag (autowrap),
    // original mode.  (Those are the ones we support at least.)

    s.saveCursor( );

  } else if ( ch == '8' ) {

    // DECRC: Restore cursor
    //
    s.restoreCursor( );

  } else if ( ch == 'D' ) {

    // Index

    if ( s.cursor_y == s.scrollRegion_bottom ) {
      s.delLine( s.scrollRegion_top );
    }
    else {
      s.cursor_y++;
    }

  } else if ( ch == 'M' ) {

    // Reverse Index

    if ( s.cursor_y == s.scrollRegion_top ) {
      s.insLine( s.scrollRegion_top );
    }
    else {
      s.cursor_y--;
    }

  }

  else if ( ch == 'E' ) {

    // NEL - Next Line

    s.adjustVertical( parms[0] );
    s.cursor_x = 0;

  } else if ( ch == 'c' ) {

    // RIS: Reset to Initial State, full reset
    s.resetTerminalState( g.useMcga );

  }

}
