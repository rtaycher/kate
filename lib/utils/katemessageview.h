/*
        katemessageview.h
        Simple message view for kate plugins
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

#ifndef _KATE_MESSAGE_VIEW_H_
#define _KATE_MESSAGE_VIEW_H_

#include "katedockviewbase.h"

/**
    A simple message view for Kate plugins.
    
    This is a message view for displaying output from processes.
    
    It uses a QTextEdit in _ mode to display the text. 
    
    You can use HTML links, for example to allow the user to go
    to a line in a precessed document.
    
    Connect to the linkClicked() signal to process the links.
    
    @section Usage
    
    To use it in the intended way:
    @li Create a KProcess (or derived class)
    @li As the output of the process arrives, hand it over
        using addText(). The view will add it to the end and
        make sure it is scrolled to the end.
    
    Each time you restart the process, clear() the view.
*/
class KateMessageView : public KateDockViewBase {
  Q_OBJECT
  public:
    KateMessageView( QWidget *parent=0, const char *name=0 );
    ~KateMessageView();
    
  public slots:
    void addMessage( const QString &msg );
    void clear();
    
    
  signals: 
    void linkClicked( const QString & href );
        
  private:
    class QTextBrowser *m_view;
};

#endif // _KATE_MESSAGE_VIEW_H_

