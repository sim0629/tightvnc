// Copyright (C) 2008,2009,2010,2011,2012 GlavSoft LLC.
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

#include "DesktopClient.h"
#include "server-config-lib/Configurator.h"
#include "desktop-ipc/UpdateHandlerClient.h"
#include "LocalUpdateHandler.h"
#include "WindowsInputBlocker.h"
#include "desktop-ipc/UserInputClient.h"
#include "SasUserInput.h"
#include "WindowsUserInput.h"
#include "DesktopConfigLocal.h"

DesktopClient::DesktopClient(ClipboardListener *extClipListener,
                       UpdateSendingListener *extUpdSendingListener,
                       AbnormDeskTermListener *extDeskTermListener,
                       LogWriter *log)
: GuiDesktop(extClipListener, extUpdSendingListener, extDeskTermListener, log),
  m_clToSrvChan(0),
  m_srvToClChan(0),
  m_clToSrvGate(0),
  m_srvToClGate(0),
  m_deskServWatcher(0),
  m_dispatcher(0),
  m_userInputClient(0),
  m_deskConf(0),
  m_gateKicker(0),
  m_log(log)
{
  m_log->info(_T("Creating DesktopClient"));

  try {
    m_deskServWatcher = new DesktopServerWatcher(this, m_log);

    // Transport initialization
    m_clToSrvChan = new ReconnectingChannel(60000, m_log);
    m_srvToClChan = new ReconnectingChannel(60000, m_log);

    m_clToSrvGate = new BlockingGate(m_clToSrvChan);
    m_srvToClGate = new BlockingGate(m_srvToClChan);

    m_dispatcher = new DesktopSrvDispatcher(m_srvToClGate, this, m_log);

    m_updateHandler = new UpdateHandlerClient(m_clToSrvGate, m_dispatcher,
                                              this, m_log);

    UserInputClient *userInputClient =
      new UserInputClient(m_clToSrvGate, m_dispatcher, this);
    m_userInputClient = userInputClient;
    m_userInput = new SasUserInput(userInputClient, m_log);

    m_deskConf = new DesktopConfigClient(m_clToSrvGate);
    m_gateKicker = new GateKicker(m_clToSrvGate);
    // Start dispatcher after handler registrations
    m_dispatcher->resume();
    onConfigReload(0);

    Configurator::getInstance()->addListener(this);
  } catch (Exception &ex) {
    m_log->error(_T("Exception during DesktopClient creaion: %s"), ex.getMessage());
    freeResource();
    throw;
  }
  resume();
}

DesktopClient::~DesktopClient()
{
  m_log->info(_T("Deleting DesktopClient"));
  terminate();
  wait();
  freeResource();
  m_log->info(_T("DesktopClient deleted"));
}

void DesktopClient::freeResource()
{
  Configurator::getInstance()->removeListener(this);

  if (m_deskServWatcher) delete m_deskServWatcher;

  closeDesktopServerTransport();

  if (m_dispatcher) delete m_dispatcher;

  if (m_gateKicker) delete m_gateKicker;
  if (m_updateHandler) delete m_updateHandler;
  if (m_deskConf) delete m_deskConf;
  if (m_userInput) delete m_userInput;
  if (m_userInputClient) delete m_userInputClient;

  if (m_srvToClGate) delete m_srvToClGate;
  if (m_clToSrvGate) delete m_clToSrvGate;

  if (m_srvToClChan) delete m_srvToClChan;
  if (m_clToSrvChan) delete m_clToSrvChan;
}

void DesktopClient::closeDesktopServerTransport()
{
  try {
    if (m_clToSrvChan) m_clToSrvChan->close();
  } catch (Exception &e) {
    m_log->error(_T("Cannot close client->server channel from Windesktop: %s"),
               e.getMessage());
  }
  try {
    if (m_srvToClChan) m_srvToClChan->close();
  } catch (Exception &e) {
    m_log->error(_T("Cannot close server->client channel from Windesktop: %s"),
               e.getMessage());
  }
}

void DesktopClient::onAnObjectEvent()
{
  m_extDeskTermListener->onAbnormalDesktopTerminate();
  m_log->error(_T("Forced closing of pipe conections"));
  closeDesktopServerTransport();
}

void DesktopClient::onReconnect(Channel *newChannelTo, Channel *newChannelFrom)
{
  BlockingGate gate(newChannelTo);
  if (m_deskConf) {
    m_log->info(_T("try update remote configuration from the ")
              _T("DesktopClient::onReconnect() function"));
    m_deskConf->updateByNewSettings(&gate);
  }
  if (m_updateHandler) {
    m_log->info(_T("try update remote UpdateHandler from the ")
              _T("DesktopClient::onReconnect() function"));
    m_updateHandler->sendInit(&gate);
  }
  if (m_userInput) {
    m_log->info(_T("try update remote UserInput from the ")
              _T("DesktopClient::onReconnect() function"));
    m_userInput->sendInit(&gate);
  }

  m_clToSrvChan->replaceChannel(newChannelTo);
  m_srvToClChan->replaceChannel(newChannelFrom);
}

void DesktopClient::onTerminate()
{
  m_newUpdateEvent.notify();
}

void DesktopClient::execute()
{
  m_log->info(_T("DesktopClient thread started"));

  while (!isTerminating()) {
    m_newUpdateEvent.waitForEvent();
    if (!isTerminating()) {
      sendUpdate();
    }
  }

  m_log->info(_T("DesktopClient thread stopped"));
}

bool DesktopClient::isRemoteInputTempBlocked()
{
  return !m_deskConf->isRemoteInputAllowed();
}

void DesktopClient::applyNewConfiguration()
{
  m_log->info(_T("reload DesktopClient configuration"));
  m_deskConf->updateByNewSettings(m_clToSrvGate);
}
