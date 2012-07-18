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

#ifndef __DESKTOPINTERFACE_H__
#define __DESKTOPINTERFACE_H__

#include "util/StringStorage.h"
#include "region/Dimension.h"
#include "rfb/PixelFormat.h"
#include "fb-update-sender/UpdateRequestListener.h"

// This class is a public interface to a desktop.
class DesktopInterface : public UpdateRequestListener
{
public:
  // Puts a current desktop name from working session to the
  // desktopName argument and an user name to userMame.
  virtual void getCurrentUserInfo(StringStorage *desktopName,
                                  StringStorage *userName) = 0;
  // Puts the current frame buffer dimension and pixel format to
  // the dim and pf function arguments.
  virtual void getFrameBufferProperties(Dimension *dim, PixelFormat *pf) = 0;

  virtual void getPrimaryDesktopCoords(Rect *rect) = 0;
  virtual void getDisplayNumberCoords(Rect *rect,
                                      unsigned char dispNumber) = 0;
  virtual void getWindowCoords(HWND hwnd, Rect *rect) = 0;
  virtual HWND getWindowHandleByName(const StringStorage *windowName) = 0;

  virtual void setKeyboardEvent(UINT32 keySym, bool down) = 0;
  virtual void setMouseEvent(UINT16 x, UINT16 y, UINT8 buttonMask) = 0;
  virtual void setNewClipText(const StringStorage *newClipboard) = 0;
};

#endif // __DESKTOPINTERFACE_H__
