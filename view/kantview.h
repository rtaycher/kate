/***************************************************************************
                          kantview.h  -  description
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

#ifndef kant_view_h
#define kant_view_h

#include "../kantmain.h"
#include "../document/kantdocument.h"

#include "../kwrite/kwview.h"
#include "../kwrite/kwattribute.h"
#include "../kwrite/kwdoc.h"
#include "../kwrite/kwdialog.h"
#include "../kwrite/highlight.h"
#include "../kwrite/kwrite_factory.h"

#include <kaction.h>

class KantView : public KWrite
{
  Q_OBJECT

  public:
    KantView (QWidget *parent = 0L, KantDocument *doc = 0L, const char * name = 0, bool HandleOwnURIDrops = false);
    ~KantView ();

    void setActive (bool b);
    bool isActive ();
    QList<KAction> bmActions() { return bookmarkActionList; }
    void doUpdateBookmarks() { updateBookmarks(); }
  private:
    bool active;

  public slots:
    virtual void setFocus ();
    void searchAgain(bool back=false);

  protected:
    bool eventFilter(QObject* o, QEvent* e);

  signals:
    void gotFocus (KantView *);
};

#endif
