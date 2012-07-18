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

#include "TvnViewer.h"
#include "network/socket/WindowsSocket.h"
#include "util/ResourceLoader.h"
#include "resource.h"

TvnViewer::TvnViewer(HINSTANCE appInstance, const TCHAR *windowClassName,
                     const TCHAR *viewerWindowClassName)
: WindowsApplication(appInstance, windowClassName),
  m_viewerWindowClassName(viewerWindowClassName),
  m_conListener(0),
  m_hAccelTable(0),
  m_logWriter(ViewerConfig::getInstance()->getLogger()),
  m_isListening(false)
{
  m_logWriter.info(_T("Init WinSock 2.1"));
  WindowsSocket::startup(2, 1);
  registerViewerWindowClass();

  // working with accelerator
  ResourceLoader *rLoader = ResourceLoader::getInstance();
  m_hAccelTable = rLoader->loadAccelerator(IDR_ACCEL_APP_KEYS);

  m_trayIcon = new ControlTrayIcon(this);
  m_loginDialog = new LoginDialog(this);
  m_instances = new ViewerCollector(this);
}

TvnViewer::~TvnViewer()
{
  m_logWriter.info(_T("Viewer collector: destroy all instances"));
  m_instances->destroyAllInstances();

  delete m_instances;
  delete m_loginDialog;
  delete m_trayIcon;

  unregisterViewerWindowClass();

  m_logWriter.info(_T("Shutdown WinSock"));
  WindowsSocket::cleanup();
}

void TvnViewer::startListening(const ConnectionConfig *conConf, const int listeningPort)
{
  if (m_isListening) {
    _ASSERT(true);
    return;
  }
  m_isListening = true;

  try {
    m_conListener = new ConnectionListener(this, listeningPort);
    m_trayIcon->showIcon();
  } catch (Exception &exception) {
    m_isListening = false;
    MessageBox(0, exception.getMessage(), _T("Application Error"), MB_OK | MB_ICONERROR);
  }
}

void TvnViewer::stopListening()
{
  if (m_isListening) {
    if (m_conListener != 0) {
      delete m_conListener;
      m_conListener = 0;
    }
    m_trayIcon->hide();
    m_isListening = false;
  }
}

void TvnViewer::addInstance(ViewerInstance *viewerInstance)
{
  m_instances->addInstance(viewerInstance);
}

void TvnViewer::runInstance(const StringStorage *hostName, const ConnectionConfig *config)
{
  ConnectionData *conData = new ConnectionData;
  conData->setHost(hostName);
  runInstance(conData, config);
}

void TvnViewer::runInstance(ConnectionData *conData, const ConnectionConfig *config)
{
  ViewerInstance *viewerInst = new ViewerInstance(this, conData, config);
  viewerInst->start();

  addInstance(viewerInst);
}

bool TvnViewer::notConnected() const
{
  return m_instances->empty();
}

bool TvnViewer::isVisibleLoginDialog() const
{
  return !!m_loginDialog->getControl()->getWindow();
}

bool TvnViewer::isListening() const
{
  return m_isListening;
}

void TvnViewer::newConnection(const StringStorage *hostName, const ConnectionConfig *config)
{
  ConnectionData *conData = new ConnectionData;
  conData->setHost(hostName);
  runInstance(conData, config);
}

void TvnViewer::newConnection(const ConnectionData *conData, const ConnectionConfig *config)
{
  ConnectionData *copyConData = new ConnectionData;
  if (!conData->isEmpty()) {
    copyConData->setHost(&conData->getHost());
  }
  copyConData->setPlainPassword(conData->getDefaultPassword());
  runInstance(copyConData, config);
}

void TvnViewer::showLoginDialog()
{
  m_loginDialog->setListening(m_isListening);
  m_loginDialog->loadIcon(IDI_APPICON);
  m_loginDialog->show();
  addModelessDialog(m_loginDialog->getControl()->getWindow());
}

void TvnViewer::createWindow(const TCHAR *className)
{
  WindowsApplication::createWindow(className);
  SetWindowLongPtr(m_mainWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

void TvnViewer::registerWindowClass(WNDCLASS *wndClass)
{
  memset(wndClass, 0, sizeof(WNDCLASS));

  wndClass->lpfnWndProc = wndProc;
  wndClass->hInstance = m_appInstance;
  wndClass->lpszClassName = m_windowClassName.getString();

  RegisterClass(wndClass);
}

void TvnViewer::registerViewerWindowClass()
{
  memset(&m_viewerWndClass, 0, sizeof(WNDCLASS));

  m_viewerWndClass.lpfnWndProc   = wndProcViewer;
  m_viewerWndClass.hInstance     = m_appInstance;
  m_viewerWndClass.lpszClassName = m_viewerWindowClassName.getString();
  m_viewerWndClass.style         = CS_HREDRAW | CS_VREDRAW;
  m_viewerWndClass.hbrBackground = GetSysColorBrush(COLOR_WINDOW);

  RegisterClass(&m_viewerWndClass);
}

void TvnViewer::unregisterViewerWindowClass()
{
  UnregisterClass(m_viewerWndClass.lpszClassName, GetModuleHandle(0));
}

LRESULT CALLBACK TvnViewer::wndProcViewer(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  BaseWindow *_this = 0;

  if (message == WM_CREATE) {
    CREATESTRUCT * createStruct = reinterpret_cast<CREATESTRUCT *>(lParam);
    BaseWindow *newBaseWindow = reinterpret_cast<BaseWindow *>(createStruct->lpCreateParams);
    SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(_this));
    newBaseWindow->setHWnd(hWnd);
    _this = newBaseWindow;
  } else {
    _this = reinterpret_cast<BaseWindow *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
  }
  if (_this != 0) {
    if (_this->wndProc(message, wParam, lParam)) {
      return 0;
    }
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

void TvnViewer::showListeningOptions()
{
  ConnectionConfigSM ccsm(RegistryPaths::VIEWER_PATH,
                          _T(".listen"));
  ConnectionConfig conConfig;
  conConfig.loadFromStorage(&ccsm);

  OptionsDialog dialog;
  dialog.setConnectionConfig(&conConfig);
  if (dialog.showModal() == 1) {
    conConfig.saveToStorage(&ccsm);
  }
}

void TvnViewer::showConfiguration()
{
  m_configurationDialog.show();
  addModelessDialog(m_configurationDialog.getControl()->getWindow());
}

void TvnViewer::showAboutViewer()
{
  m_aboutDialog.show();
  addModelessDialog(m_aboutDialog.getControl()->getWindow());
}

int TvnViewer::processMessages()
{
  MSG msg;
  BOOL ret;
  while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0) {
    if (ret < 0) {
      return 1;
    }
    if (m_hAccelTable && ret != 0) {
      if (TranslateAccelerator(GetActiveWindow(), m_hAccelTable, &msg)) {
        continue;
      }
    }

    if (!processDialogMessage(&msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  return (int)msg.wParam;
}

void TvnViewer::newListeningConnection()
{
  ConnectionData connectionData;

  ConnectionConfig connectionConfig;
  ConnectionConfigSM ccsm(RegistryPaths::VIEWER_PATH, _T(".listen"));
  connectionConfig.loadFromStorage(&ccsm);

  if (m_conListener != 0) {
    SocketIPv4 *socket = m_conListener->getNewConnection();
    while (socket != 0) {
      ViewerInstance *viewerInst = new ViewerInstance(this,
                                                      &connectionData,
                                                      &connectionConfig,
                                                      socket);
      viewerInst->start();
      addInstance(viewerInst);

      socket = m_conListener->getNewConnection();
    }
  }
}

LRESULT CALLBACK TvnViewer::wndProc(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  if (msg >= WM_USER) {
    TvnViewer *_this = reinterpret_cast<TvnViewer *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (_this != 0) {
      switch (msg) {
      case WM_USER_NEW_LISTENING:
        _this->newListeningConnection();
        break;
      case WM_USER_NEW_CONNECTION:
        _this->showLoginDialog();
        break;
      }
    }
    return true;
  } else {
    return WindowsApplication::wndProc(hWnd, msg, wparam, lparam);
  }
}
