/***************************************************************************
                          toolviewmanager.h -  description
                             -------------------
    begin                : Sat June 16 2002
    copyright            : (C) 2001 by Joseph Wenninger
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

#include <qwidget.h>
#include <kurl.h>

#include <kdockwidget.h>

namespace Kate
{

/** An interface to the kate toolviewmanager.

 */
class ToolViewManager /* You can assume that the implementation is always at least an QObject derived class too */
{

  protected:
    ToolViewManager ();
    virtual ~ToolViewManager ();
      
  public:

	// The removeToolView* methods take care of the dockwidget and the widget deletion
	// This makes it possible to let the widget inherit KXMLGUIClient without crashes
        // After Kate has returned to the main event loop the widget is going to be destroyed.
	// Don't use any pointers still referencing it and NEVER delete it yourself !!!!!!!!!

	//The dockwidgets name is "DOCK"+widget->name() Please make sure that this is unique
	//IMPORTANT: YOU MUST SPECIFY A PIXMAP
    virtual class KDockWidget *addToolViewWidget(KDockWidget::DockPosition,QWidget *widget,const class QPixmap&, const class QString&)=0;
    virtual bool  removeToolViewWidget(QWidget *widget)=0;

	//Please make sure that the name is unique
	//IMPORTANT: YOU MUST SPECIFY A PIXMAP
    virtual class KDockWidget *addToolView(KDockWidget::DockPosition pos,const char *name,const QPixmap &icon,const QString&)=0;
    virtual bool removeToolView(KDockWidget *)=0;

    /* the following methods aren't used yet */
    virtual bool hideToolView(class KDockWidget*)=0;
    virtual bool showToolView(class KDockWidget*)=0;
    virtual bool hideToolView(const QString& name)=0;
    virtual bool showToolView(const QString& name)=0;
    
};

};

#endif
