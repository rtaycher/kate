/***************************************************************************
                          kantpartview.h  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
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

#ifndef KANTPARTVIEW_H
#define KANTPARTVIEW_H

#include "../kantmain.h"

#include "../view/kantview.h"

class KAction;
class KToggleAction;
class KRecentFilesAction;
class KSelectAction;

class KantPartView : public KantView
{
  Q_OBJECT

  friend class KantPartDocument;

  public:
    KantPartView (KantPartDocument *doc=0L, QWidget *parent = 0L, const char * name = 0, bool HandleOwnURIDrops = true);
    ~KantPartView ();

    void setupActions();

    KAction *fileSave, *editInsert, *editCut, *editPaste,
            *editReplace, *editUndo, *editRedo, *editUndoHist,
            *toolsIndent, *toolsUnindent, *toolsCleanIndent,
            *toolsComment, *toolsUncomment, *toolsSpell;

    KToggleAction *setVerticalSelection;
    KRecentFilesAction *fileRecent;
    KSelectAction *setHighlight, *setEndOfLine;

  protected slots:
    void slotDropEventPass( QDropEvent * ev );

  public slots:
    void slotUpdate();
    void slotFileStatusChanged();
    void slotNewUndo();
    void slotHighlightChanged();
};

#endif
