/***************************************************************************
                          view.h -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph "Crossfire" Cullmann
    email                : crossfire@babylon2k.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KATE_VIEW_INCLUDE_
#define _KATE_VIEW_INCLUDE_

#include <ktexteditor.h>

class KConfig;

namespace Kate
{

class Document;

class View : public KTextEditor::View
{
  Q_OBJECT

  public:
    View ( KTextEditor::Document *doc, QWidget *parent, const char *name = 0 );
    virtual ~View ();

    virtual Document *getDoc () { return 0L; };

    virtual QString markedText () { return 0L; };

  public slots:
    virtual void configDialog () { ; };

    virtual void setHl (int) { ; };
    virtual int getHl () { return 0; };
    virtual int getHlCount () { return 0; };
    virtual QString getHlName (int) { return 0L; };

  public:
    virtual void readConfig () { ; };
    virtual void writeConfig () { ; };

    virtual void readSessionConfig (KConfig *) { ; };
    virtual void writeSessionConfig (KConfig *) { ; };

  public slots:
    virtual void keyReturn () { ; };
    virtual void keyDelete () { ; };
    virtual void backspace () { ; };
    virtual void killLine () { ; };

    virtual void cursorLeft () { ; };
    virtual void shiftCursorLeft () { ; };
    virtual void cursorRight () { ; };
    virtual void shiftCursorRight () { ; };
    virtual void wordLeft () { ; };
    virtual void shiftWordLeft () { ; };
    virtual void wordRight () { ; };
    virtual void shiftWordRight () { ; };
    virtual void home () { ; };
    virtual void shiftHome () { ; };
    virtual void end () { ; };
    virtual void shiftEnd () { ; };
    virtual void up () { ; };
    virtual void shiftUp () { ; };
    virtual void down () { ; };
    virtual void shiftDown () { ; };
    virtual void scrollUp () { ; };
    virtual void scrollDown () { ; };
    virtual void topOfView () { ; };
    virtual void bottomOfView () { ; };
    virtual void pageUp () { ; };
    virtual void shiftPageUp () { ; };
    virtual void pageDown () { ; };
    virtual void shiftPageDown () { ; };
    virtual void top () { ; };
    virtual void shiftTop () { ; };
    virtual void bottom () { ; };
    virtual void shiftBottom () { ; };
};

};

#endif
