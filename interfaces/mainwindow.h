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

#ifndef _KATE_MAINWINDOW_INCLUDE_
#define _KATE_MAINWINDOW_INCLUDE_

#include <qobject.h>

#include <kxmlguifactory.h>
#include <kurl.h>

namespace Kate
{

class Project;
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
    Kate::ViewManager *viewManager () const;

  public :

    class ToolViewManager *toolViewManager() const;

  public slots: /*these are slots for kjs*/
    /**
     * Returns the active project of this main window
     * @return Project current active project
     */
    Kate::Project *activeProject () const;

    /**
     * Creates a new project file at give url of given type + opens it
     * @param type projecttype
     * @param filename name of the new project file
     * @return Project new created project object
     */
    Kate::Project *createProject (const QString &type, const QString &name, const QString &filename);

    /**
     * @param filename name of the project file
     * @return Project opened project
     */
    Kate::Project *openProject (const QString &filename);

  //invention of public signals, like in kparts/browserextension.h
  #undef signals
  #define signals public
  signals:
  #undef signals
  #define signals protected

    void projectChanged ();

  private:
    class PrivateMainWindow *d;
};

}

#endif
