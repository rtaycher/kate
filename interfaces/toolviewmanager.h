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

namespace KMDI
{
  class ToolViewAccessor;
}

namespace Kate
{

/**
  Interface to the toolviewmanager
 */
class KDE_EXPORT ToolViewManager : public QObject
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
     * @param tabToolTip tooltip for the tab
     * @param tabCaption caption for the tab
     * @return KMDI::ToolViewAccessor * generated accessor
     */
    KMDI::ToolViewAccessor *addToolView (KDockWidget::DockPosition position, QWidget *widget, const QPixmap &icon, const QString &sname, const QString &tabToolTip = 0, const QString &tabCaption = 0);

    /**
     * Remove a toolview
     * @param toolview widget to remove
     * @return bool success
     */
    bool removeToolView (QWidget *widget);

    /**
     * Remove a toolview
     * @param toolview to remove
     * @return bool success
     */
    bool removeToolView (KMDI::ToolViewAccessor *accessor);

    /**
     * Show the toolview
     * @param widget to show
     * @return bool success
     */
    bool showToolView (QWidget *widget);

    /**
     * Show the toolview
     * @param toolview to show
     * @return bool success
     */
    bool showToolView (KMDI::ToolViewAccessor *accessor);

    /**
     * Hide the toolview
     * @param widget to hide
     * @return bool success
     */
    bool hideToolView (QWidget *widget);

    /**
     * Hide the toolview
     * @param toolview to hide
     * @return bool success
     */
    bool hideToolView (KMDI::ToolViewAccessor *accessor);

  private:
    /**
     * REALLY PRIVATE ;)
     */
    class PrivateToolViewManager *d;
};

}

#endif
