/***************************************************************************
                          toolviewmanager.h -  description
                             -------------------
    begin                : Sat June 16 2002
    copyright            : (C) 2002 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
 ***************************************************************************/

#ifndef _KATE_TOOLVIEWMANAGER_INCLUDE_
#define _KATE_TOOLVIEWMANAGER_INCLUDE_

#include <kdockwidget.h>

namespace Kate
{

/** An interface to the kate toolviewmanager.

 */
class ToolViewManager : public QObject
{
  friend class PrivateToolViewManager;

  Q_OBJECT
  
  public:
    ToolViewManager ( void *toolViewManager );
    virtual ~ToolViewManager ();
      
  public:

	// The removeToolView* methods take care of the dockwidget and the widget deletion
	// This makes it possible to let the widget inherit KXMLGUIClient without crashes
        // After Kate has returned to the main event loop the widget is going to be destroyed.
	// Don't use any pointers still referencing it and NEVER delete it yourself !!!!!!!!!

	//The dockwidgets name is "DOCK"+widget->name() Please make sure that this is unique
	//IMPORTANT: YOU MUST SPECIFY A PIXMAP
    KDockWidget *addToolViewWidget(KDockWidget::DockPosition position,QWidget *widget, const class QPixmap&icon, const class QString&sname);
    bool removeToolViewWidget(QWidget *widget);

	//Please make sure that the name is unique
	//IMPORTANT: YOU MUST SPECIFY A PIXMAP
    KDockWidget *addToolView(KDockWidget::DockPosition position,const char *name,const QPixmap &icon,const QString&sname);
    bool removeToolView(KDockWidget *dockwidget);

    /* the following methods aren't used yet */
    bool hideToolView(KDockWidget*dockwidget);
    bool showToolView(KDockWidget*dockwidget);
    bool hideToolView(const QString& sname);
    bool showToolView(const QString& sname);
    
  private:
    class PrivateToolViewManager *d;  
};

};

#endif
