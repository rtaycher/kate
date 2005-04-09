/* This file is part of the KDE project
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>

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

#include "katesession.h"
#include "katesession.moc"

#include "kateapp.h"
#include "katemainwindow.h"

#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kdebug.h>

KateSessionManager::KateSessionManager(QObject *parent) : QObject (parent)
{
  setupSessionList ();
}

KateSessionManager::~KateSessionManager()
{
  for (unsigned int i=0; i < m_sessionList.size(); ++i)
    delete m_sessionList[i];
}

KateSessionManager *KateSessionManager::self()
{
  return KateApp::self()->kateSessionManager ();
}

void KateSessionManager::setupSessionList ()
{
  // Let's get a list of all session we have atm
  QStringList listRel;
  QStringList list = KGlobal::dirs()->findAllResources("data", "kate/sessions/*.katesession", false, true, listRel);

  for (unsigned int i=0; i < list.count(); ++i)
  {
    KateSession *session = new KateSession ();
    session->sessionFile = list[i];
    session->sessionFileLocal = locateLocal( "data", listRel[i]);

    KSimpleConfig config (session->sessionFile, true);
    config.setGroup ("General");
    session->sessionName = config.readEntry ("Name", i18n ("Unknown"));

    m_sessionList.append (session);

    kdDebug () << "FOUND SESSION: " << session->sessionName << " FILE TO SAVE: " << session->sessionFileLocal << endl;
  }
}
