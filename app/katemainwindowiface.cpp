/* This file is part of the KDE project
   Copyright (C) 2003 Ian Reinhart Geiser <geiseri@kde.org>

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

#include "katemainwindowiface.h"

#include "katemainwindow.h"

#include <kdebug.h>

KateMainWindowDCOPIface::KateMainWindowDCOPIface (KateMainWindow *w) : DCOPObject ((QString("KateMainWindow#%1").arg(w->mainWindowNumber())).latin1()), m_w (w)
{
}

DCOPRef KateMainWindowDCOPIface::activeProject () const
{
  Kate::Project *p = m_w->activeProject ();

  if (p)
    return DCOPRef (p->dcopObject ());

  return DCOPRef ();
}

DCOPRef KateMainWindowDCOPIface::createProject (QString type, QString name, QString filename)
{
  Kate::Project *p = m_w->createProject (type, name, filename);

  if (p)
    return DCOPRef (p->dcopObject ());

  return DCOPRef ();
}

DCOPRef KateMainWindowDCOPIface::openProject (QString filename)
{
  Kate::Project *p = m_w->openProject (filename);

  if (p)
    return DCOPRef (p->dcopObject ());

  return DCOPRef ();
}
