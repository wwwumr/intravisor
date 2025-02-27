/* Determine display width of Unicode character.
   Copyright (C) 2001-2002, 2006-2018 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2002.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as published
   by the Free Software Foundation; either version 2.1 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "uniwidth.h"

#include "cjk.h"

/*
 * Non-spacing attribute table.
 * Consists of:
 * - Non-spacing characters; generated from PropList.txt or
 *   "grep '^[^;]*;[^;]*;[^;]*;[^;]*;NSM;' UnicodeData.txt"
 * - Format control characters; generated from
 *   "grep '^[^;]*;[^;]*;Cf;' UnicodeData.txt"
 * - Zero width characters; generated from
 *   "grep '^[^;]*;ZERO WIDTH ' UnicodeData.txt"
 */
static const unsigned char nonspacing_table_data[38*64] = {
  /* 0x0000-0x01ff */
  0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, /* 0x0000-0x003f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, /* 0x0040-0x007f */
  0xff, 0xff, 0xff, 0xff, 0x00, 0x20, 0x00, 0x00, /* 0x0080-0x00bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x00c0-0x00ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0100-0x013f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0140-0x017f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0180-0x01bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x01c0-0x01ff */
  /* 0x0200-0x03ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0200-0x023f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0240-0x027f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0280-0x02bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x02c0-0x02ff */
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 0x0300-0x033f */
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, /* 0x0340-0x037f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0380-0x03bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x03c0-0x03ff */
  /* 0x0400-0x05ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0400-0x043f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0440-0x047f */
  0xf8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0480-0x04bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x04c0-0x04ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0500-0x053f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0540-0x057f */
  0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xbf, /* 0x0580-0x05bf */
  0xb6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x05c0-0x05ff */
  /* 0x0600-0x07ff */
  0x3f, 0x00, 0xff, 0x17, 0x00, 0x00, 0x00, 0x00, /* 0x0600-0x063f */
  0x00, 0xf8, 0xff, 0xff, 0x00, 0x00, 0x01, 0x00, /* 0x0640-0x067f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0680-0x06bf */
  0x00, 0x00, 0xc0, 0xbf, 0x9f, 0x3d, 0x00, 0x00, /* 0x06c0-0x06ff */
  0x00, 0x80, 0x02, 0x00, 0x00, 0x00, 0xff, 0xff, /* 0x0700-0x073f */
  0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0740-0x077f */
  0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x01, 0x00, /* 0x0780-0x07bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x0f, 0x00, /* 0x07c0-0x07ff */
  /* 0x0800-0x09ff */
  0x00, 0x00, 0xc0, 0xfb, 0xef, 0x3e, 0x00, 0x00, /* 0x0800-0x083f */
  0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, /* 0x0840-0x087f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0880-0x08bf */
  0x00, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, /* 0x08c0-0x08ff */
  0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, /* 0x0900-0x093f */
  0xfe, 0x21, 0xfe, 0x00, 0x0c, 0x00, 0x00, 0x00, /* 0x0940-0x097f */
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, /* 0x0980-0x09bf */
  0x1e, 0x20, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, /* 0x09c0-0x09ff */
  /* 0x0a00-0x0bff */
  0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, /* 0x0a00-0x0a3f */
  0x86, 0x39, 0x02, 0x00, 0x00, 0x00, 0x23, 0x00, /* 0x0a40-0x0a7f */
  0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, /* 0x0a80-0x0abf */
  0xbe, 0x21, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, /* 0x0ac0-0x0aff */
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, /* 0x0b00-0x0b3f */
  0x1e, 0x20, 0x40, 0x00, 0x0c, 0x00, 0x00, 0x00, /* 0x0b40-0x0b7f */
  0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0b80-0x0bbf */
  0x01, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0bc0-0x0bff */
  /* 0x0c00-0x0dff */
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, /* 0x0c00-0x0c3f */
  0xc1, 0x3d, 0x60, 0x00, 0x0c, 0x00, 0x00, 0x00, /* 0x0c40-0x0c7f */
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, /* 0x0c80-0x0cbf */
  0x00, 0x30, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, /* 0x0cc0-0x0cff */
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0d00-0x0d3f */
  0x1e, 0x20, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, /* 0x0d40-0x0d7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0d80-0x0dbf */
  0x00, 0x04, 0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0dc0-0x0dff */
  /* 0x0e00-0x0fff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf2, 0x07, /* 0x0e00-0x0e3f */
  0x80, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0e40-0x0e7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf2, 0x1b, /* 0x0e80-0x0ebf */
  0x00, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0ec0-0x0eff */
  0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0xa0, 0x02, /* 0x0f00-0x0f3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x7f, /* 0x0f40-0x0f7f */
  0xdf, 0xe0, 0xff, 0xfe, 0xff, 0xff, 0xff, 0x1f, /* 0x0f80-0x0fbf */
  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x0fc0-0x0fff */
  /* 0x1000-0x11ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xfd, 0x66, /* 0x1000-0x103f */
  0x00, 0x00, 0x00, 0xc3, 0x01, 0x00, 0x1e, 0x00, /* 0x1040-0x107f */
  0x64, 0x20, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, /* 0x1080-0x10bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10c0-0x10ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1100-0x113f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1140-0x117f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1180-0x11bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11c0-0x11ff */
  /* 0x1200-0x13ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1200-0x123f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1240-0x127f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1280-0x12bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x12c0-0x12ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1300-0x133f */
  0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, /* 0x1340-0x137f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1380-0x13bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x13c0-0x13ff */
  /* 0x1600-0x17ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1600-0x163f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1640-0x167f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1680-0x16bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16c0-0x16ff */
  0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x1c, 0x00, /* 0x1700-0x173f */
  0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x00, /* 0x1740-0x177f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 0x3f, /* 0x1780-0x17bf */
  0x40, 0xfe, 0x0f, 0x20, 0x00, 0x00, 0x00, 0x00, /* 0x17c0-0x17ff */
  /* 0x1800-0x19ff */
  0x00, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1800-0x183f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1840-0x187f */
  0x60, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, /* 0x1880-0x18bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x18c0-0x18ff */
  0x00, 0x00, 0x00, 0x00, 0x87, 0x01, 0x04, 0x0e, /* 0x1900-0x193f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1940-0x197f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1980-0x19bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x19c0-0x19ff */
  /* 0x1a00-0x1bff */
  0x00, 0x00, 0x80, 0x09, 0x00, 0x00, 0x00, 0x00, /* 0x1a00-0x1a3f */
  0x00, 0x00, 0x40, 0x7f, 0xe5, 0x1f, 0xf8, 0x9f, /* 0x1a40-0x1a7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x7f, /* 0x1a80-0x1abf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1ac0-0x1aff */
  0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x17, /* 0x1b00-0x1b3f */
  0x04, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x0f, 0x00, /* 0x1b40-0x1b7f */
  0x03, 0x00, 0x00, 0x00, 0x3c, 0x3b, 0x00, 0x00, /* 0x1b80-0x1bbf */
  0x00, 0x00, 0x00, 0x00, 0x40, 0xa3, 0x03, 0x00, /* 0x1bc0-0x1bff */
  /* 0x1c00-0x1dff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xcf, 0x00, /* 0x1c00-0x1c3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1c40-0x1c7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1c80-0x1cbf */
  0x00, 0x00, 0xf7, 0xff, 0xfd, 0x21, 0x10, 0x03, /* 0x1cc0-0x1cff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d00-0x1d3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d40-0x1d7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d80-0x1dbf */
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xf8, /* 0x1dc0-0x1dff */
  /* 0x2000-0x21ff */
  0x00, 0xf8, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00, /* 0x2000-0x203f */
  0x00, 0x00, 0x00, 0x00, 0xdf, 0xff, 0x00, 0x00, /* 0x2040-0x207f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2080-0x20bf */
  0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, /* 0x20c0-0x20ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2100-0x213f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2140-0x217f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2180-0x21bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x21c0-0x21ff */
  /* 0x2c00-0x2dff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2c00-0x2c3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2c40-0x2c7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2c80-0x2cbf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x00, /* 0x2cc0-0x2cff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2d00-0x2d3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, /* 0x2d40-0x2d7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x2d80-0x2dbf */
  0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, /* 0x2dc0-0x2dff */
  /* 0x3000-0x31ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, /* 0x3000-0x303f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x3040-0x307f */
  0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, /* 0x3080-0x30bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x30c0-0x30ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x3100-0x313f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x3140-0x317f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x3180-0x31bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x31c0-0x31ff */
  /* 0xa600-0xa7ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa600-0xa63f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xf7, 0x3f, /* 0xa640-0xa67f */
  0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, /* 0xa680-0xa6bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, /* 0xa6c0-0xa6ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa700-0xa73f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa740-0xa77f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa780-0xa7bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa7c0-0xa7ff */
  /* 0xa800-0xa9ff */
  0x44, 0x08, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, /* 0xa800-0xa83f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa840-0xa87f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa880-0xa8bf */
  0x30, 0x00, 0x00, 0x00, 0xff, 0xff, 0x03, 0x00, /* 0xa8c0-0xa8ff */
  0x00, 0x00, 0x00, 0x00, 0xc0, 0x3f, 0x00, 0x00, /* 0xa900-0xa93f */
  0x80, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xa940-0xa97f */
  0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x13, /* 0xa980-0xa9bf */
  0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, /* 0xa9c0-0xa9ff */
  /* 0xaa00-0xabff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x66, 0x00, /* 0xaa00-0xaa3f */
  0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, /* 0xaa40-0xaa7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9d, 0xc1, /* 0xaa80-0xaabf */
  0x02, 0x00, 0x00, 0x00, 0x00, 0x30, 0x40, 0x00, /* 0xaac0-0xaaff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xab00-0xab3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xab40-0xab7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xab80-0xabbf */
  0x00, 0x00, 0x00, 0x00, 0x20, 0x21, 0x00, 0x00, /* 0xabc0-0xabff */
  /* 0xfa00-0xfbff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfa00-0xfa3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfa40-0xfa7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfa80-0xfabf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfac0-0xfaff */
  0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, /* 0xfb00-0xfb3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfb40-0xfb7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfb80-0xfbbf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfbc0-0xfbff */
  /* 0xfe00-0xffff */
  0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, /* 0xfe00-0xfe3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfe40-0xfe7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xfe80-0xfebf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, /* 0xfec0-0xfeff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xff00-0xff3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xff40-0xff7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xff80-0xffbf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, /* 0xffc0-0xffff */
  /* 0x10000-0x101ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10000-0x1003f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10040-0x1007f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10080-0x100bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x100c0-0x100ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10100-0x1013f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10140-0x1017f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10180-0x101bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, /* 0x101c0-0x101ff */
  /* 0x10200-0x103ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10200-0x1023f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10240-0x1027f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10280-0x102bf */
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, /* 0x102c0-0x102ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10300-0x1033f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, /* 0x10340-0x1037f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10380-0x103bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x103c0-0x103ff */
  /* 0x10a00-0x10bff */
  0x6e, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87, /* 0x10a00-0x10a3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10a40-0x10a7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10a80-0x10abf */
  0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, /* 0x10ac0-0x10aff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10b00-0x10b3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10b40-0x10b7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10b80-0x10bbf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10bc0-0x10bff */
  /* 0x11000-0x111ff */
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, /* 0x11000-0x1103f */
  0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, /* 0x11040-0x1107f */
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x26, /* 0x11080-0x110bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x110c0-0x110ff */
  0x07, 0x00, 0x00, 0x00, 0x80, 0xef, 0x1f, 0x00, /* 0x11100-0x1113f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, /* 0x11140-0x1117f */
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x7f, /* 0x11180-0x111bf */
  0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x111c0-0x111ff */
  /* 0x11200-0x113ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xd3, 0x40, /* 0x11200-0x1123f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11240-0x1127f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11280-0x112bf */
  0x00, 0x00, 0x00, 0x80, 0xf8, 0x07, 0x00, 0x00, /* 0x112c0-0x112ff */
  0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, /* 0x11300-0x1133f */
  0x01, 0x00, 0x00, 0x00, 0xc0, 0x1f, 0x1f, 0x00, /* 0x11340-0x1137f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11380-0x113bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x113c0-0x113ff */
  /* 0x11400-0x115ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, /* 0x11400-0x1143f */
  0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11440-0x1147f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x85, /* 0x11480-0x114bf */
  0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x114c0-0x114ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11500-0x1153f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11540-0x1157f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0xb0, /* 0x11580-0x115bf */
  0x01, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, /* 0x115c0-0x115ff */
  /* 0x11600-0x117ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xa7, /* 0x11600-0x1163f */
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11640-0x1167f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0xbf, 0x00, /* 0x11680-0x116bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x116c0-0x116ff */
  0x00, 0x00, 0x00, 0xe0, 0xbc, 0x0f, 0x00, 0x00, /* 0x11700-0x1173f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11740-0x1177f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11780-0x117bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x117c0-0x117ff */
  /* 0x11c00-0x11dff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x3f, /* 0x11c00-0x11c3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11c40-0x11c7f */
  0x00, 0x00, 0xfc, 0xff, 0xff, 0xfc, 0x6d, 0x00, /* 0x11c80-0x11cbf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11cc0-0x11cff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11d00-0x11d3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11d40-0x11d7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11d80-0x11dbf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x11dc0-0x11dff */
  /* 0x16a00-0x16bff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16a00-0x16a3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16a40-0x16a7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16a80-0x16abf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, /* 0x16ac0-0x16aff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, /* 0x16b00-0x16b3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16b40-0x16b7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16b80-0x16bbf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16bc0-0x16bff */
  /* 0x16e00-0x16fff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16e00-0x16e3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16e40-0x16e7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16e80-0x16ebf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16ec0-0x16eff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16f00-0x16f3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16f40-0x16f7f */
  0x00, 0x80, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16f80-0x16fbf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x16fc0-0x16fff */
  /* 0x1bc00-0x1bdff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1bc00-0x1bc3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1bc40-0x1bc7f */
  0x00, 0x00, 0x00, 0x60, 0x0f, 0x00, 0x00, 0x00, /* 0x1bc80-0x1bcbf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1bcc0-0x1bcff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1bd00-0x1bd3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1bd40-0x1bd7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1bd80-0x1bdbf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1bdc0-0x1bdff */
  /* 0x1d000-0x1d1ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d000-0x1d03f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d040-0x1d07f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d080-0x1d0bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d0c0-0x1d0ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d100-0x1d13f */
  0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0xf8, 0xff, /* 0x1d140-0x1d17f */
  0xe7, 0x0f, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, /* 0x1d180-0x1d1bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d1c0-0x1d1ff */
  /* 0x1d200-0x1d3ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d200-0x1d23f */
  0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d240-0x1d27f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d280-0x1d2bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d2c0-0x1d2ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d300-0x1d33f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d340-0x1d37f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d380-0x1d3bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1d3c0-0x1d3ff */
  /* 0x1da00-0x1dbff */
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xf8, /* 0x1da00-0x1da3f */
  0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x20, 0x00, /* 0x1da40-0x1da7f */
  0x10, 0x00, 0x00, 0xf8, 0xfe, 0xff, 0x00, 0x00, /* 0x1da80-0x1dabf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1dac0-0x1daff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1db00-0x1db3f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1db40-0x1db7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1db80-0x1dbbf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1dbc0-0x1dbff */
  /* 0x1e000-0x1e1ff */
  0x7f, 0xff, 0xff, 0xf9, 0xdb, 0x07, 0x00, 0x00, /* 0x1e000-0x1e03f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e040-0x1e07f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e080-0x1e0bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e0c0-0x1e0ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e100-0x1e13f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e140-0x1e17f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e180-0x1e1bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e1c0-0x1e1ff */
  /* 0x1e800-0x1e9ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e800-0x1e83f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e840-0x1e87f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e880-0x1e8bf */
  0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e8c0-0x1e8ff */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e900-0x1e93f */
  0xf0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e940-0x1e97f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x1e980-0x1e9bf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  /* 0x1e9c0-0x1e9ff */
};
static const signed char nonspacing_table_ind[248] = {
   0,  1,  2,  3,  4,  5,  6,  7, /* 0x0000-0x0fff */
   8,  9, -1, 10, 11, 12, 13, -1, /* 0x1000-0x1fff */
  14, -1, -1, -1, -1, -1, 15, -1, /* 0x2000-0x2fff */
  16, -1, -1, -1, -1, -1, -1, -1, /* 0x3000-0x3fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x4000-0x4fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x5000-0x5fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x6000-0x6fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x7000-0x7fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x8000-0x8fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x9000-0x9fff */
  -1, -1, -1, 17, 18, 19, -1, -1, /* 0xa000-0xafff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0xb000-0xbfff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0xc000-0xcfff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0xd000-0xdfff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0xe000-0xefff */
  -1, -1, -1, -1, -1, 20, -1, 21, /* 0xf000-0xffff */
  22, 23, -1, -1, -1, 24, -1, -1, /* 0x10000-0x10fff */
  25, 26, 27, 28, -1, -1, 29, -1, /* 0x11000-0x11fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x12000-0x12fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x13000-0x13fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x14000-0x14fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x15000-0x15fff */
  -1, -1, -1, -1, -1, 30, -1, 31, /* 0x16000-0x16fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x17000-0x17fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x18000-0x18fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x19000-0x19fff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x1a000-0x1afff */
  -1, -1, -1, -1, -1, -1, 32, -1, /* 0x1b000-0x1bfff */
  -1, -1, -1, -1, -1, -1, -1, -1, /* 0x1c000-0x1cfff */
  33, 34, -1, -1, -1, 35, -1, -1, /* 0x1d000-0x1dfff */
  36, -1, -1, -1, 37, -1, -1, -1  /* 0x1e000-0x1efff */
};

/* Determine number of column positions required for UC.  */
int
uc_width (ucs4_t uc, const char *encoding)
{
  /* Test for non-spacing or control character.  */
  if ((uc >> 9) < 248)
    {
      int ind = nonspacing_table_ind[uc >> 9];
      if (ind >= 0)
        if ((nonspacing_table_data[64*ind + ((uc >> 3) & 63)] >> (uc & 7)) & 1)
          {
            if (uc > 0 && uc < 0xa0)
              return -1;
            else
              return 0;
          }
    }
  else if ((uc >> 9) == (0xe0000 >> 9))
    {
      if (uc >= 0xe0100)
        {
          if (uc <= 0xe01ef)
            return 0;
        }
      else
        {
          if (uc >= 0xe0020 ? uc <= 0xe007f : uc == 0xe0001)
            return 0;
        }
    }
  /* Test for double-width character.
   * Generated from "grep '^[^;]\{4,5\};[WF]' EastAsianWidth.txt"
   * and            "grep '^[^;]\{4,5\};[^WF]' EastAsianWidth.txt"
   */
  if (uc >= 0x1100
      && ((uc < 0x1160) /* Hangul Jamo */
          || (uc >= 0x2329 && uc < 0x232b) /* Angle Brackets */
          || (uc >= 0x2e80 && uc < 0xa4d0  /* CJK ... Yi */
              && !(uc == 0x303f) && !(uc >= 0x4dc0 && uc < 0x4e00))
          || (uc >= 0xac00 && uc < 0xd7a4) /* Hangul Syllables */
          || (uc >= 0xf900 && uc < 0xfb00) /* CJK Compatibility Ideographs */
          || (uc >= 0xfe10 && uc < 0xfe20) /* Presentation Forms for Vertical */
          || (uc >= 0xfe30 && uc < 0xfe70) /* CJK Compatibility Forms */
          || (uc >= 0xff00 && uc < 0xff61) /* Fullwidth Forms */
          || (uc >= 0xffe0 && uc < 0xffe7) /* Fullwidth Signs */
          || (uc >= 0x20000 && uc <= 0x2ffff) /* Supplementary Ideographic Plane */
          || (uc >= 0x30000 && uc <= 0x3ffff) /* Tertiary Ideographic Plane */
     )   )
    return 2;
  /* In ancient CJK encodings, Cyrillic and most other characters are
     double-width as well.  */
  if (uc >= 0x00A1 && uc < 0xFF61 && uc != 0x20A9
      && is_cjk_encoding (encoding))
    return 2;
  return 1;
}
