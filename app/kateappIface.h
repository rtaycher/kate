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

#ifndef _kateapp_Iface_h_
#define _kateapp_Iface_h_

#include <dcopobject.h>
#include <dcopref.h>
#include <kurl.h>

class KateApp;

class KateAppDCOPIface : public DCOPObject
{
  K_DCOP

  public:
    KateAppDCOPIface (KateApp *app);

  k_dcop:
    DCOPRef documentManager ();

    DCOPRef activeMainWindow ();

    uint activeMainWindowNumber ();

    uint mainWindows ();
    DCOPRef mainWindow (uint n = 0);

    /**
     * open a file with given url and encoding
     * will get view created
     * @param url url of the file
     * @param encoding encoding name
     * @return success
     */
    bool openURL (KURL url, QString encoding);

    /**
     * set cursor of active view in active main window
     * @param line line for cursor
     * @param column column for cursor
     * @return success
     */
    bool setCursor (int line, int column);

    /**
     * helper to handle stdin input
     * open a new document/view, fill it with the text given
     * @param text text to fill in the new doc/view
     * @return success
     */
    bool openInput (QString text);

    /**
     * activate a given session
     * @param session session name
     * @return success
     */
    bool activateSession (QString session);

  private:
    KateApp *m_app;
};
#endif
