/***************************************************************************
                          katemaildialog.cpp
                          Misc dialogs and dialog pages for Kate
                             -------------------
    begin                : Wed Mar 06 2002
    copyright            : (C) 2002 by Anders Lund
    email                : anders@alweb.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "katemailfilesdialog.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "katedocmanager.h"

#include <klistview.h>
#include <klocale.h>
#include <kurl.h>

#include <qevent.h>
#include <qlabel.h>
#include <qstringlist.h>
#include <qvbox.h>
 
/* a private check list item, that can store a Kate::Document*.  */
class KateDocCheckItem : public QCheckListItem {
  public:
    KateDocCheckItem( QListView *parent, const QString& text, Kate::Document *d )
      : QCheckListItem( parent, text, QCheckListItem::CheckBox ), mdoc(d) {};
    Kate::Document *doc() { return mdoc; };
  private:
    Kate::Document *mdoc;
};

///////////////////////////////////////////////////////////////////////////
// KateMailDialog implementation
///////////////////////////////////////////////////////////////////////////
KateMailDialog::KateMailDialog( QWidget *parent, KateMainWindow  *mainwin )
  : KDialogBase( parent, "kate mail dialog", true, i18n("Email File(s)"),
                Ok|Cancel|User1, Ok, false,
                KGuiItem( i18n("&Show All Documents >>") ) ),
    mainWindow( mainwin )
{
  setButtonOKText( i18n("&Mail...") );
  mw = makeVBoxMainWidget();
  mw->installEventFilter( this );

  lInfo = new QLabel( i18n(
        "<p>Press <strong>Mail...</strong> to email the current document."
        "<p>To select more documents to send, press <strong>Show All Documents&nbsp;&gt;&gt;</strong>."), mw );
  // TODO avoid untill needed - later
  list = new KListView( mw );
  list->addColumn( i18n("Name") );
  list->addColumn( i18n("URL") );
  Kate::Document *currentDoc = mainWindow->m_viewManager->activeView()->getDoc();
  uint n = mainWindow->m_docManager->documents();
  uint i = 0;
  QCheckListItem *item;
  while ( i < n ) {
    Kate::Document *doc = mainWindow->m_docManager->document( i );
    if ( doc ) {
      item = new KateDocCheckItem( list, doc->docName(), doc );
      item->setText( 1, doc->url().prettyURL() );
      if ( doc == currentDoc ) {
        item->setOn( true );
        item->setSelected( true );
      }
    }
    i++;
  }
  list->hide();
  connect( this, SIGNAL(user1Clicked()), this, SLOT(slotShowButton()) );
  mw->setMinimumSize( lInfo->sizeHint() );
}

QPtrList<Kate::Document> KateMailDialog::selectedDocs()
{
  QPtrList<Kate::Document> l;
  QListViewItem *item = list->firstChild();
  while ( item ) {
    if ( ((KateDocCheckItem*)item)->isOn() )
      l.append( ((KateDocCheckItem*)item)->doc() );
    item = item->nextSibling();
  }
  return l;
}

void KateMailDialog::slotShowButton()
{
  if ( list->isVisible() ) {
    setButtonText( User1, i18n("&Show All Documents >>") );
    list->hide();
  }
  else {
    list->show();
    setButtonText( User1, i18n("&Hide Document List <<") );
    lInfo->setText( i18n("Press <strong>Mail...</strong> to send selected documents") );

  }
  mw->setMinimumSize( QSize( lInfo->sizeHint().width(), mw->sizeHint().height()) );
  setMinimumSize( calculateSize( mw->minimumSize().width(), mw->sizeHint().height() ) );
  resize( width(), minimumHeight() );
}
