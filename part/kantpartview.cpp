/***************************************************************************
                          kantpartview.cpp  -  description
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

#include "kantpartview.h"
#include "kantpartview.moc"

#include "kantpartdocument.h"
#include "kantpartfactory.h"

#include <kaction.h>
#include <kstdaction.h>
#include <klocale.h>
#include <kdebug.h>

KantPartView::KantPartView (KantPartDocument *doc, QWidget *parent, const char * name, bool HandleOwnURIDrops):  KantView (doc, parent, name, HandleOwnURIDrops)
{
  setInstance( KantPartFactory::instance() );

  setupActions();

  setXMLFile( "kantpartui.rc" );

  connect( this, SIGNAL( newStatus() ), this, SLOT( slotUpdate() ) );
  connect( this, SIGNAL( newUndo() ), this, SLOT( slotNewUndo() ) );
  connect( this, SIGNAL( fileChanged() ), this, SLOT( slotFileStatusChanged() ) );
  connect( doc, SIGNAL( highlightChanged() ), this, SLOT( slotHighlightChanged() ) );

  setHighlight->setCurrentItem(getHl());
  slotUpdate();
}

KantPartView::~KantPartView ()
{
  if ( !myDoc->m_bSingleViewMode )
  {
    if ( myDoc->isLastView(0))
      delete myDoc;
  }
}

void KantPartView::setupActions()
{
    KStdAction::openNew(this, SLOT(newDoc()), actionCollection());
    KStdAction::open(this, SLOT(open()), actionCollection());
    fileRecent = KStdAction::openRecent(this, SLOT(slotOpenRecent(const KURL&)),
                                        actionCollection());

    fileSave = KStdAction::save(this, SLOT(save()), actionCollection());
    KStdAction::saveAs(this, SLOT(saveAs()), actionCollection());

    // setup edit menu
    editUndo = KStdAction::undo(this, SLOT(undo()), actionCollection());
    editRedo = KStdAction::redo(this, SLOT(redo()), actionCollection());
    editUndoHist = new KAction(i18n("Undo/Redo &History..."), 0, this, SLOT(undoHistory()),
                               actionCollection(), "edit_undoHistory");
    editCut = KStdAction::cut(this, SLOT(cut()), actionCollection());
    editPaste = KStdAction::copy(this, SLOT(copy()), actionCollection());
    editReplace = KStdAction::paste(this, SLOT(paste()), actionCollection());
    KStdAction::selectAll(this, SLOT(selectAll()), actionCollection());
    new KAction(i18n("&Deselect All"), 0, this, SLOT(deselectAll()),
                actionCollection(), "edit_deselectAll");
    new KAction(i18n("Invert &Selection"), 0, this, SLOT(invertSelection()),
                actionCollection(), "edit_invertSelection");
    KStdAction::find(this, SLOT(find()), actionCollection());
    KStdAction::findNext(this, SLOT(findAgain()), actionCollection());
    KStdAction::replace(this, SLOT(replace()), actionCollection());
    editInsert = new KAction(i18n("&Insert File..."), 0, this, SLOT(insertFile()),
                             actionCollection(), "edit_insertFile");

    // setup Go menu
    KStdAction::gotoLine(this, SLOT(gotoLine()), actionCollection());
    KAction *addAct = new KAction(i18n("&Add Marker"), Qt::CTRL+Qt::Key_M, this, SLOT(addBookmark()),
                                  actionCollection(), "go_addMarker");
    connect(this, SIGNAL(bookAddChanged(bool)),addAct,SLOT(setEnabled(bool)));
    new KAction(i18n("&Set Marker..."), 0, this, SLOT(setBookmark()),
                actionCollection(), "go_setMarker");
    KAction *clearAct = new KAction(i18n("&Clear Markers"), 0, this, SLOT(clearBookmarks()),
                                    actionCollection(), "go_clearMarkers");
    connect(this, SIGNAL(bookClearChanged(bool)),clearAct,SLOT(setEnabled(bool)));
    clearAct->setEnabled(false);

    // setup Tools menu
    toolsSpell = KStdAction::spelling(this, SLOT(spellcheck()), actionCollection());
    toolsIndent = new KAction(i18n("&Indent"), Qt::CTRL+Qt::Key_I, this, SLOT(indent()),
                              actionCollection(), "tools_indent");
    toolsUnindent = new KAction(i18n("&Unindent"), Qt::CTRL+Qt::Key_U, this, SLOT(unIndent()),
                                actionCollection(), "tools_unindent");
    toolsCleanIndent = new KAction(i18n("&Clean Indentation"), 0, this, SLOT(cleanIndent()),
                                   actionCollection(), "tools_cleanIndent");
    toolsComment = new KAction(i18n("C&omment"), 0, this, SLOT(comment()),
                               actionCollection(), "tools_comment");
    toolsUncomment = new KAction(i18n("Unco&mment"), 0, this, SLOT(uncomment()),
                                 actionCollection(), "tools_uncomment");

    new KAction(i18n("Configure Highlighti&ng..."), 0, this, SLOT(hlDlg()),actionCollection(), "set_confHighlight");

    setVerticalSelection = new KToggleAction(i18n("&Vertical Selection"), 0, this, SLOT(toggleVertical()),
                                             actionCollection(), "set_verticalSelect");

    setHighlight = new KSelectAction(i18n("&Highlight Mode"), 0, actionCollection(), "set_highlight");
    connect(setHighlight, SIGNAL(activated(int)), this, SLOT(setHl(int)));
    QStringList list;
    for (int z = 0; z < HlManager::self()->highlights(); z++)
        list.append(i18n(HlManager::self()->hlName(z)));
    setHighlight->setItems(list);

    setEndOfLine = new KSelectAction(i18n("&End Of Line"), 0, actionCollection(), "set_eol");
    connect(setEndOfLine, SIGNAL(activated(int)), this, SLOT(setEol(int)));
    list.clear();
    list.append("&Unix");
    list.append("&Macintosh");
    list.append("&Windows/Dos");
    setEndOfLine->setItems(list);
}

void KantPartView::slotUpdate()
{
    int cfg = config();
    bool readOnly = isReadOnly();

    setVerticalSelection->setChecked(cfg & KantPartView::cfVerticalSelect);

    fileSave->setEnabled(!readOnly);
    editInsert->setEnabled(!readOnly);
    editCut->setEnabled(!readOnly);
    editPaste->setEnabled(!readOnly);
    editReplace->setEnabled(!readOnly);
    toolsIndent->setEnabled(!readOnly);
    toolsUnindent->setEnabled(!readOnly);
    toolsCleanIndent->setEnabled(!readOnly);
    toolsComment->setEnabled(!readOnly);
    toolsUncomment->setEnabled(!readOnly);
    toolsSpell->setEnabled(!readOnly);

    slotNewUndo();
}
void KantPartView::slotFileStatusChanged()
{
  int eol = getEol()-1;
  eol = eol>=0? eol: 0;

    setEndOfLine->setCurrentItem(eol);

    if ( !doc()->url().isEmpty() )
        //set recent files popup menu
        fileRecent->addURL(doc()->url());

}
void KantPartView::slotNewUndo()
{
    int state = undoState();

    editUndoHist->setEnabled(state & 1 || state & 2);

    QString t = i18n("Und&o");   // it would be nicer to fetch the original string
    if (state & 1) {
        editUndo->setEnabled(true);
        t += ' ';
        t += i18n(undoTypeName(nextUndoType()));
    } else {
        editUndo->setEnabled(false);
    }
    editUndo->setText(t);

    t = i18n("Re&do");   // it would be nicer to fetch the original string
    if (state & 2) {
        editRedo->setEnabled(true);
        t += ' ';
        t += i18n(undoTypeName(nextRedoType()));
    } else {
        editRedo->setEnabled(false);
    }
    editRedo->setText(t);
}

void KantPartView::slotHighlightChanged()
{
    setHighlight->setCurrentItem(getHl());
}
