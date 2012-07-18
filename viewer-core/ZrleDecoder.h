// Copyright (C) 2011,2012 GlavSoft LLC.
// All rights reserved.
//
//-------------------------------------------------------------------------
// This file is part of the TightVNC software.  Please visit our Web site:
//
//                       http://www.tightvnc.com/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//-------------------------------------------------------------------------
//

#ifndef _ZRLE_DECODER_H_
#define _ZRLE_DECODER_H_

#include "util/Inflater.h"

#include "Decoder.h"

class ZrleDecoder : public Decoder
{
public:
  ZrleDecoder(LogWriter *logWriter);
  virtual ~ZrleDecoder();

  void decode(RfbInputGate *input,
              FrameBuffer *frameBuffer,
              const Rect *dstRect);

  int getCode() const;

private:
  typedef vector<unsigned int> Palette;

  void inflate(RfbInputGate *input, size_t unpackedSize);

  int readType(const vector<unsigned char> &out, size_t *const readed);

  size_t readRunLength(const vector<unsigned char> &out,
                       size_t *const readed);

  Palette readPalette(const vector<unsigned char> &out,
                      size_t *readed,
                      const int paletteSize);

  void readRawTile(const vector<unsigned char> &out,
                   size_t *const readed,
                   vector<char> &pixels,
                   const Rect *tileRect);

  void readSolidTile(const vector<unsigned char> &out,
                     size_t *const readed,
                     vector<char> &pixels,
                     const Rect *tileRect);

  void readPackedPaletteTile(const vector<unsigned char> &out,
                             size_t *const readed,
                             vector<char> &pixels,
                             const Rect *tileRect,
                             const int type);

  void readPlainRleTile(const vector<unsigned char> &out,
                        size_t *const readed,
                        vector<char> &pixels,
                        const Rect *tileRect);

  void readPaletteRleTile(const vector<unsigned char> &out,
                          size_t *const readed,
                          vector<char> &pixels,
                          const Rect *tileRect,
                          const int type);


  void drawTile(FrameBuffer *fb,
                const Rect *tileRect,
                const vector<char> *pixels);

  Inflater m_inflater;
  size_t m_bytesPerPixel;
  size_t m_numberFirstByte;

private:
  static const int TILE_SIZE = 64;

  // maximal size of tile (64x64) is max value from follow:
  // 1. size of raw: subenc + data:
  //    1 + width * height * pixelSize = 1 + 64 * 64 * 4 = 16385
  // 2. size of solid: subenc + solidColor:
  //    1 + pixelSize = 1 + 4 = 5
  // 3. size of packpalette: subenc + palleteSize + data:
  //    1 + paletteSize * pixelSize + m = 1 + 16 * 4 + 32 * 64 = 2113
  // 4. size of plainRle: subenc + rle * width * height:
  //    1 + (pixelSize + 1) * width * height = 1 + (4 + 1) * 64 * 64 = 20481
  // 5. size of paletteRle: subenc + paletteSzie + data:
  //    1 + paletteSize * pixelSize + rle * width * height = 1 + 128 * 4 + (1 + 1) * 64 * 64 = 8705
  static const size_t MAXIMAL_TILE_SIZE = 20481;
  static const size_t TILE_LENGTH_SIZE = sizeof(UINT32);

  static size_t getMaxSizeOfRectangle(const Rect *dstRect);
};

#endif
