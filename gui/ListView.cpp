// Copyright (C) 2009,2010,2011,2012 GlavSoft LLC.
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

#include "ListView.h"

void ListView::addColumn(int index, const TCHAR *caption, int width, int fmt)
{
  //
  // Create LV_COLUMN struct
  //

  LV_COLUMN lvColumn = {0};
  lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
  lvColumn.fmt = fmt;
  lvColumn.cx = width;
  lvColumn.pszText = (TCHAR *)caption;

  //
  // Add column to list view
  //

  ListView_InsertColumn(m_hwnd, index, &lvColumn);
}

void ListView::addColumn(int index, const TCHAR *caption, int width)
{
  addColumn(index, caption, width, LVCFMT_LEFT);
}

ListViewItem ListView::getItem(int index)
{
  // Output structure
  ListViewItem item;
  // Windows list view item concept
  LVITEM lvI;
  // Buffer for text data
  TCHAR textBuffer[256];

  //
  // Retrieve text and tag from list view item (zero subitem)
  //

  lvI.mask = LVIF_TEXT | LVIF_PARAM;
  lvI.pszText = (TCHAR *)&textBuffer[0];
  lvI.iItem = index;
  lvI.iSubItem = 0;

  //
  // FIXME: Forced set text limit. 256 chars max
  //

  lvI.cchTextMax = 256 * sizeof(TCHAR);

  // Trying to get data from window
  ListView_GetItem(m_hwnd, &lvI);

  //
  // Copying data to our list view item structure
  //

  item.index = lvI.iItem;
  item.tag = lvI.lParam;

  return item;
}

void ListView::addItem(int index, const TCHAR *caption)
{
  addItem(index, caption, NULL);
}

void ListView::addItem(int index, const TCHAR *caption, LPARAM tag)
{
  //
  // Prepare LVITEM structure
  //

  LVITEM lvI;
  lvI.mask = LVIF_TEXT | LVIF_PARAM;
  lvI.lParam = tag;
  lvI.iItem = index;
  lvI.iSubItem = 0;
  lvI.pszText = (TCHAR*)caption;

  //
  // Send message to window
  //

  ListView_InsertItem(m_hwnd, &lvI);
}

void ListView::addItem(int index, const TCHAR *caption, LPARAM tag, int imageIndex)
{
  //
  // Prepare LVITEM structure
  //

  LVITEM lvI;
  lvI.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
  lvI.lParam = tag;
  lvI.iItem = index;
  lvI.iSubItem = 0;
  lvI.iImage = imageIndex;
  lvI.pszText = (TCHAR*)caption;

  //
  // Send message to window
  //

  ListView_InsertItem(m_hwnd, &lvI);
}

void ListView::removeItem(int i)
{
  ListView_DeleteItem(getWindow(), i);
}

void ListView::clear()
{
  ListView_DeleteAllItems(m_hwnd);
}

void ListView::setSubItemText(int index, int subIndex, const TCHAR *caption)
{
  //
  // Prepare LVITEM structure
  //

  LVITEM lvI;
  lvI.mask = LVIF_TEXT;
  lvI.iItem = index;
  lvI.iSubItem = subIndex;
  lvI.pszText = (TCHAR*)caption;

  //
  // Send message to window
  //

  SendMessage(m_hwnd, LVM_SETITEM, 0, (LPARAM)&lvI);
}

void ListView::setItemData(int index, LPARAM tag)
{
  //
  // Prepare LVITEM structure
  //

  LVITEM lvI;
  lvI.mask = LVIF_PARAM;
  lvI.lParam = tag;
  lvI.iItem = index;
  lvI.iSubItem = 0;
  lvI.lParam = tag;

  //
  // Send message to window
  //

  SendMessage(m_hwnd, LVM_SETITEM, 0, (LPARAM)&lvI);
}

LPARAM ListView::getItemData(int index)
{
  return getItem(index).tag;
}

ListViewItem ListView::getSelectedItem()
{
  ListViewItem item;
  int index = getSelectedIndex();
  if (index == -1) {
    item.index = -1;
    item.tag = NULL;
  } else {
    item = getItem(index);
  }
  return item;
}

int ListView::getSelectedIndex()
{
  int iSelect = ListView_GetNextItem(m_hwnd, -1, LVNI_SELECTED);
  return iSelect;
}

void ListView::selectItem(int index)
{
  WPARAM itemIndex = (WPARAM)index;
  ListView_SetItemState(m_hwnd, -1, 0, LVIS_SELECTED);
  SendMessage(m_hwnd, LVM_ENSUREVISIBLE , itemIndex, FALSE);
  ListView_SetItemState(m_hwnd, itemIndex, LVIS_SELECTED, LVIS_SELECTED);
  ListView_SetItemState(m_hwnd, itemIndex, LVIS_FOCUSED, LVIS_FOCUSED);
  setFocus();
}

void ListView::setFullRowSelectStyle(bool fullRowSelect)
{
  if (fullRowSelect) {
    addExStyle(LVS_EX_FULLROWSELECT);
  } else {
    removeExStyle(LVS_EX_FULLROWSELECT);
  }
}

void ListView::allowMultiSelection(bool allow)
{
  if (allow) {
    removeStyle(LVS_SINGLESEL);
  } else {
    addStyle(LVS_SINGLESEL);
  }
}

unsigned int ListView::getSelectedItemsCount()
{
  return ListView_GetSelectedCount(m_hwnd);
}

void ListView::getSelectedItemsIndexes(int *indexes)
{
  int i = -1;
  for (unsigned int j = 0; j < getSelectedItemsCount(); j++) {
    i = ListView_GetNextItem(m_hwnd, i, LVNI_SELECTED);
    indexes[j] = i;
  }
}

void ListView::setExStyle(DWORD style) 
{
  ::SendMessage(m_hwnd, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)style); 
}

DWORD ListView::getExStyle()
{
  return ListView_GetExtendedListViewStyle(m_hwnd);
}

void ListView::addExStyle(DWORD styleFlag)
{
  DWORD flags = getExStyle();
  flags |= styleFlag;
  setExStyle(flags);
}

void ListView::removeExStyle(DWORD styleFlag)
{
  DWORD flags = getExStyle();
  flags &= ~styleFlag;
  setExStyle(flags);
}
