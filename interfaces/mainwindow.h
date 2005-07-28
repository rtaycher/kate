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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KATE_MAINWINDOW_INCLUDE_
#define _KATE_MAINWINDOW_INCLUDE_

#include <qobject.h>

#include <kxmlguifactory.h>
#include <kurl.h>

namespace Kate
{

class ViewManager;

class KDE_EXPORT MainWindow : public QObject
{
  friend class PrivateMainWindow;

  Q_OBJECT

  public:
    MainWindow (void *mainWindow);
    virtual ~MainWindow ();

  public: /*these are slots for kjs*/
    KXMLGUIFactory *guiFactory() const;

  public slots:
    ViewManager *viewManager () const;

  public :
    /**
     * @return the kate main window.
     */
    class QWidget *window() const;
    /**
     * Access the widget (in the middle of the four sidebars) in which the
     * editor component and the KateTabBar are embedded. This widget is a KVBox,
     * so other child widgets can be embedded under the editor widget.
     *
     * @return the central widget
     */
    class QWidget *centralWidget() const;
    /**
     * Access the toolview manager. The toolview manager manages the four
     * sidebars and provides an interface to create/remove new sidebar entries.
     * @return the toolview manager
     */
    class ToolViewManager *toolViewManager() const;

  private:
    class PrivateMainWindow *d;
};

}

#endif
