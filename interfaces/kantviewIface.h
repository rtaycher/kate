 /***************************************************************************
                          kantpluginiface.h  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _KANT_VIEW_IFACE_
#define _KANT_VIEW_IFACE_

#include <ktexteditor.h>
#include <qstring.h>

class KantViewIface : public KTextEditor::View
{
  Q_OBJECT

  public:
    KantViewIface( KTextEditor::Document *doc, QWidget *parent, const char *name = 0 ) : KTextEditor::View (doc, parent, name) {;};
    virtual ~KantViewIface () {;};

    virtual QString markedText()=0;

  public slots:
    virtual void keyReturn() { ; };
    virtual void keyDelete() { ; };
    virtual void backspace() { ; };
    virtual void killLine() { ; };

    virtual void cursorLeft() { ; };
    virtual void shiftCursorLeft() { ; };
    virtual void cursorRight() { ; };
    virtual void shiftCursorRight() { ; };
    virtual void wordLeft() { ; };
    virtual void shiftWordLeft() { ; };
    virtual void wordRight() { ; };
    virtual void shiftWordRight() { ; };
    virtual void home() { ; };
    virtual void shiftHome() { ; };
    virtual void end() { ; };
    virtual void shiftEnd() { ; };
    virtual void up() { ; };
    virtual void shiftUp() { ; };
    virtual void down() { ; };
    virtual void shiftDown() { ; };
    virtual void scrollUp() { ; };
    virtual void scrollDown() { ; };
    virtual void topOfView() { ; };
    virtual void bottomOfView() { ; };
    virtual void pageUp() { ; };
    virtual void shiftPageUp() { ; };
    virtual void pageDown() { ; };
    virtual void shiftPageDown() { ; };
    virtual void top() { ; };
    virtual void shiftTop() { ; };
    virtual void bottom() { ; };
    virtual void shiftBottom() { ; };
};

#endif
