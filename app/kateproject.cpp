/* This file is part of the KDE project
   Copyright (C) 2002 Christoph Cullmann <cullmann@kde.org>

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

// $Id$

#include "kateproject.h"
#include "kateproject.moc"

#include "kateprojectmanager.h"

#include <kio/netaccess.h>

//
// INTERNAL CLASS FOR PROJECT MANAGER ONLY !!!!!!!!!!!!!
//

KateProject::KateProject (KateProjectManager *proMan, QObject *parent, const KURL &url) : QObject (parent)
{
  m_url = url;
  m_tmpFile = 0;
  m_projectMan = proMan;
    
  if (m_url.isLocalFile())
    m_data = new KConfig (m_url.path());
  else
  {
    m_tmpFile = new KTempFile (QString::null, ".kateproject");
    m_tmpFile->setAutoDelete (true);
    QString tmp = m_tmpFile->name();
    KIO::NetAccess::download(m_url, tmp);
    
    m_data = new KConfig (m_tmpFile->name());
  }
  
  m_project = new Kate::Project (this);
  m_plugin = m_projectMan->createPlugin (m_project);
}

KateProject::~KateProject()
{
  delete m_data;
  
  if (m_tmpFile)
    delete m_tmpFile;
}

QString KateProject::type () const
{
  m_data->setGroup("General");
  return m_data->readEntry ("Type", "Default");
}

QString KateProject::name () const
{
  m_data->setGroup("General");
  return m_data->readEntry ("Name", "");
}

KURL KateProject::url () const
{
  return m_url;
}

bool KateProject::save ()
{
  m_data->sync();
  
  if (!m_url.isLocalFile())
  {
    bool b = KIO::NetAccess::upload(m_tmpFile->name(), m_url);
  
    if (!b)
      return false;
  }

  return m_plugin->save ();
}

QStringList KateProject::subdirs (const QString &dir) const
{
  if (dir.isNull())
    m_data->setGroup("General");
  else
  {
    QString groupname = QString ("Dir ") + dir;

    if (!m_data->hasGroup(groupname))
      return QStringList ();

    m_data->setGroup(groupname);
  }
  
  return m_data->readListEntry ("Subdirs");
}

QStringList KateProject::files (const QString &dir) const
{
  if (dir.isNull())
    m_data->setGroup("General");
  else
  {
    QString groupname = QString ("Dir ") + dir;

    if (!m_data->hasGroup(groupname))
      return QStringList ();

    m_data->setGroup(groupname);
  }
  
  return m_data->readListEntry ("Files");
}
