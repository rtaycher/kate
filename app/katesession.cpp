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
#include <kdirwatch.h>

#include <qdir.h>

KateSession::KateSession (KateSessionManager *manager, const QString &fileName, const QString &name)
  : m_sessionFileRel (fileName)
  , m_sessionName (name)
  , m_manager (manager)
{
  bool ex (!fileName.isEmpty() && KGlobal::dirs()->exists(sessionFile ()));

  if (ex)
  {
    if (m_sessionName.isEmpty())
    {
      KSimpleConfig config (sessionFile (), true);
      config.setGroup ("General");
      m_sessionName = config.readEntry ("Name", i18n ("Unnamed Session"));
    }
  }
  else if (!fileName.isEmpty())
  {
    // create the file, write name to it!
    if (m_sessionName.isEmpty())
      m_sessionName =  i18n ("Unnamed Session");

    KSimpleConfig config (sessionFile ());
    config.setGroup ("General");
    config.writeEntry ("Name", m_sessionName);
    config.sync ();
  }
}

KateSession::~KateSession ()
{
}

QString KateSession::sessionFile () const
{
  return m_manager->sessionsDir() + "/" + m_sessionFileRel;
}

KateSessionManager::KateSessionManager (QObject *parent)
 : QObject (parent)
 , m_sessionsDir (locateLocal( "data", "kate/sessions"))
 , m_dirWatch (new KDirWatch (this))
 , m_activeSession (this, "", "")
{
  kdDebug() << "LOCAL SESSION DIR: " << m_sessionsDir << endl;

  // create dir if needed
  KGlobal::dirs()->makeDir (m_sessionsDir);

  // add this dir to the watch
  connect (m_dirWatch, SIGNAL(dirty (const QString &)), this, SLOT(dirty (const QString &)));
  m_dirWatch->addDir (m_sessionsDir);

  // initial setup of the sessions list
  updateSessionList ();
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

void KateSessionManager::dirty (const QString &path)
{
  updateSessionList ();
}

void KateSessionManager::updateSessionList ()
{
  for (unsigned int i=0; i < m_sessionList.size(); ++i)
    delete m_sessionList[i];

  m_sessionList.clear ();

  // Let's get a list of all session we have atm
  QDir dir (m_sessionsDir, "*.katesession");

  for (unsigned int i=0; i < dir.count(); ++i)
  {
    KateSession *session = new KateSession (this, dir[i], "");
    m_sessionList.append (session);

    kdDebug () << "FOUND SESSION: " << session->sessionName() << " FILE: " << session->sessionFile() << endl;
  }
}

void KateSessionManager::activateSession (const QString &name)
{
}
