/*
        katedockviewbase.h
        Base class for dock views
        Copyright (C) 2002 by Anders Lund <anders@alweb.dk>

        $Id:$
        ---

        This program is free software; you can redistribute it and/or
        modify it under the terms of the GNU General Public License
        as published by the Free Software Foundation; either version 2
        of the License, or (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef _KATE_DOCKVIEW_BASE_H_
#define _KATE_DOCKVIEW_BASE_H_

#include <qvbox.h>

/**
    Base class for dockwidget views
    This class creates a widget meant to be set as the main widget
    for a KDockWidget.
    
    This class provides a title in two parts, titlePrefix and title. The
    size policies are set so that neither will prevent resizing the view to
    a width smaller than the texts, the titlePrefix (on the left) will remain
    fully visible for as long as possible.
    
    Use the titlePrefix to describe the nature of the view contents, for
    example "Messages".
        
    Use the title to inform the user what is currently in the view,
    for example the name of a related file, a command or similar.
    
    To add widgets, just create them with this as the parent.
    
    @author Anders Lund <anders@alweb.dk>
*/
class KateDockViewBase : public QVBox {
  Q_OBJECT
  public:
  /**
      Create a KateDockViewBase.
  */
  KateDockViewBase( QWidget *parent=0, const char *name=0 );
  
  /**
      Create a KateDockViewBase with the title prefix @p prefix
      and the title @p title.
  */
  KateDockViewBase( const QString &prefix, const QString &title, QWidget *parent=0, const char *name=0 );
  
  ~KateDockViewBase();
  
  /**
      Set the title prefix to @p prefix.
  */
  void setTitlePrefix( const QString &prefix );
  
  /**
      @return The title prefix.
  */
  QString titlePrefix() const;
  
  /**
      Set the title to @p title 
  */
  void setTitle( const QString &title );
  
  /**
     Convenience method, sets both the prefix and title
  */
  void setTitle( const QString &prefix, const QString &title );
  
  /**
      @return the title of the KateDockViewBase
  */
  QString title() const;
  
  private:
  /** Private initialization */
  void init( const QString &, const QString &);
  class KateDockViewBasePrivate *d;
};

#endif // _KATE_DOCKVIEW_BASE_H_
