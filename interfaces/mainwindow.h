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
#include <qpixmap.h>

#include <kxmlguifactory.h>
#include <kurl.h>

class QWidget;

namespace KTextEditor { class View; }

namespace Kate
{

class KDE_EXPORT MainWindow : public QObject
{
  friend class PrivateMainWindow;

  Q_OBJECT

  public:
    MainWindow (void *mainWindow);
    virtual ~MainWindow ();

  public: /*these are slots for kjs*/
    KXMLGUIFactory *guiFactory() const;

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
   * View stuff, here all stuff belong which allows to
   * access and manipulate the KTextEditor::View's we have in this windows
   */
 public slots: /*these are slots for kjs*/
    /**
     * Returns a pointer to the currently active view
     * @return View active view
     */
    KTextEditor::View *activeView ();

    /**
     * Activates the view with the corresponding documentNumber
     * @param documentNumber the document's number
     */
    void activateView ( uint documentNumber );

    /**
     * Opens the file pointed to by URL
     * @param url url to the file
     */
    void openURL (const KURL &url);     

#ifndef Q_MOC_RUN 
  #undef signals
  #define signals public
#endif
  signals:
#ifndef Q_MOC_RUN
  #undef signals
  #define signals protected   
#endif

    /**
     * Active view has changed
     */
    void viewChanged ();

  /**
   * ToolView stuff, here all stuff belong which allows to
   * add/remove and manipulate the toolview of this main windows
   */
  public:
    /**
     * positions
     */
    enum Position { Left = 0, Right = 1, Top = 2, Bottom = 3 };

    /**
     * add a given widget to the given sidebar if possible, name is very important
     * @param identifier unique identifier for this toolview
     * @param pos position for the toolview, if we are in session restore, this is only a preference
     * @param icon icon to use for the toolview
     * @param text text to use in addition to icon
     * @return created toolview on success or 0
     */
    QWidget *createToolView (const QString &identifier, MainWindow::Position pos, const QPixmap &icon, const QString &text);

    /**
     * Move the toolview
     * @param widget to show, widget given must be widget constructed by createToolView
     * @param pos position to move widget to
     * @return bool success
     */
    bool moveToolView (QWidget *widget, MainWindow::Position pos);

    /**
     * Show the toolview
     * @param widget to show, widget given must be widget constructed by createToolView
     * @return bool success
     */
    bool showToolView (QWidget *widget);

    /**
     * Hide the toolview
     * @param widget to hide, widget given must be widget constructed by createToolView
     * @return bool success
     */
    bool hideToolView (QWidget *widget);

  private:
    class PrivateMainWindow *d;
};

}

#endif
