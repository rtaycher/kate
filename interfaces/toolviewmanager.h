/* This file is part of the KDE project
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
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

#ifndef _KATE_TOOLVIEWMANAGER_INCLUDE_
#define _KATE_TOOLVIEWMANAGER_INCLUDE_

#include <kdockwidget.h>

namespace Kate
{

/**
  Interface to the toolviewmanager
 */
class ToolViewManager : public QObject
{
  friend class PrivateToolViewManager;

  Q_OBJECT
  
  public:
    /**
     * Construtor, should not interest, internal usage
     */
    ToolViewManager (void *toolViewManager);
    
    /**
     * Desctructor
     */
    virtual ~ToolViewManager ();
      
  public:
    /**
     * Add a toolview
     * @param position position where to dock
     * @param widget widget to add
     * @param icon icon for the dock button
     * @param sname unique name (used for example for hide/show)
     * @return KDockWidget * generated dockwidget
     */
    KDockWidget *addToolViewWidget (KDockWidget::DockPosition position,QWidget *widget, const class QPixmap&icon, const class QString&sname);
    
    /**
     * Add a toolview
     * @param position position where to dock
     * @param name name
     * @param icon icon for the dock button
     * @param sname unique name (used for example for hide/show)
     * @return KDockWidget * generated dockwidget
     */
    KDockWidget *addToolView (KDockWidget::DockPosition position,const char *name,const QPixmap &icon,const QString&sname);
    
    /**
     * Remove a toolview
     * @param widget widget to remove
     * @return bool success
     */
    bool removeToolViewWidget (QWidget *widget);
    
    /**
     * Remove a toolview
     * @param dockwidget widget to remove
     * @return bool success
     */
    bool removeToolView (KDockWidget *dockwidget);

    /**
     * Show the toolview
     * @param dockwidget widget to show
     * @return bool success
     */
    bool showToolView (KDockWidget*dockwidget);
    
    /**
     * Show the toolview
     * @param sname name of the widget to show
     * @return bool success
     */
    bool showToolView (const QString& sname);
    
    /**
     * Hide the toolview
     * @param dockwidget widget to hide
     * @return bool success
     */
    bool hideToolView (KDockWidget*dockwidget);
    
    /**
     * Hide the toolview
     * @param sname name of the widget to hide
     * @return bool success
     */
    bool hideToolView (const QString& sname);

  private:
    /**
     * REALLY PRIVATE ;)
     */
    class PrivateToolViewManager *d;  
};

};

#endif
