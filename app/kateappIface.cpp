/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kateappIface.h"

#include "kateapp.h"
#include "katesession.h"
#include "katedocmanager.h"
#include "katemainwindow.h"

#include <kdebug.h>

KateAppDCOPIface::KateAppDCOPIface (KateApp *app) : DCOPObject ("KateApplication")
     , m_app (app)
{
}

DCOPRef KateAppDCOPIface::documentManager ()
{
  return DCOPRef (m_app->documentManager()->dcopObject ());
}

DCOPRef KateAppDCOPIface::activeMainWindow ()
{
  KateMainWindow *win = m_app->activeMainWindow();

  if (win)
    return DCOPRef (win->dcopObject ());

  return DCOPRef ();
}

uint KateAppDCOPIface::activeMainWindowNumber ()
{
  KateMainWindow *win = m_app->activeMainWindow();

  if (win)
    return win->mainWindowNumber ();

  return 0;
}


uint KateAppDCOPIface::mainWindows ()
{
  return m_app->mainWindows ();
}

DCOPRef KateAppDCOPIface::mainWindow (uint n)
{
  KateMainWindow *win = m_app->mainWindow(n);

  if (win)
    return DCOPRef (win->dcopObject ());

  return DCOPRef ();
}

bool KateAppDCOPIface::openURL (KURL url, QString encoding)
{
  return m_app->openURL (url, encoding);
}

bool KateAppDCOPIface::setCursor (int line, int column)
{
  return m_app->setCursor (line, column);
}

bool KateAppDCOPIface::activateSession (QString session)
{
  m_app->sessionManager()->activateSession (m_app->sessionManager()->giveSession (session));

  return true;
}
