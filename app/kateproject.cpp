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

//
// INTERNAL CLASS FOR PROJECT MANAGER ONLY !!!!!!!!!!!!!
//

KateProject::KateProject (KateProjectManager *proMan, QObject *parent, const QString &filename) : QObject (parent)
{
  m_projectMan = proMan;
  m_filename = filename;
  m_data = new KConfig (filename);
  m_project = new Kate::Project (this);
  m_plugin = m_projectMan->createPlugin (m_project);
}

KateProject::~KateProject()
{
  delete m_data;
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

QString KateProject::fileName () const
{
  return m_filename;
}

QString KateProject::dir () const
{
  m_data->setGroup("General");
  return m_data->readEntry ("Dir", "");
}

bool KateProject::save ()
{
  m_data->sync();

  return m_plugin->save ();
}

QStringList KateProject::sections () const
{
  m_data->setGroup("General");
  return m_data->readListEntry ("Sections");
}

QStringList KateProject::files (const QString &section) const
{
  if (!m_data->hasGroup(section))
    return QStringList ();

  m_data->setGroup(section);
  return m_data->readListEntry ("Files");
}
