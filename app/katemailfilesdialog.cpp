/* This file is part of the KDE project
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

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
#include <q3vbox.h>
//Added by qt3to4:
#include <Q3PtrList>

/* a private check list item, that can store a KTextEditor::Document*.  */
class KateDocCheckItem : public Q3CheckListItem {
  public:
    KateDocCheckItem( Q3ListView *parent, const QString& text, KTextEditor::Document *d )
      : Q3CheckListItem( parent, text, Q3CheckListItem::CheckBox ), mdoc(d) {};
    KTextEditor::Document *doc() { return mdoc; };
  private:
    KTextEditor::Document *mdoc;
};

///////////////////////////////////////////////////////////////////////////
// KateMailDialog implementation
///////////////////////////////////////////////////////////////////////////
KateMailDialog::KateMailDialog( QWidget *parent, KateMainWindow  *mainwin )
  : KDialogBase( parent, "kate mail dialog", true, i18n("Email Files"),
                Ok|Cancel|User1, Ok, false,
                KGuiItem( i18n("&Show All Documents >>") ) ),
    mainWindow( mainwin )
{
  setButtonGuiItem( KDialogBase::Ok, KGuiItem( i18n("&Mail..."), "mail_send") );
  mw = makeVBoxMainWidget();
  mw->installEventFilter( this );

  lInfo = new QLabel( i18n(
        "<p>Press <strong>Mail...</strong> to email the current document."
        "<p>To select more documents to send, press <strong>Show All Documents&nbsp;&gt;&gt;</strong>."), mw );
  // TODO avoid untill needed - later
  list = new KListView( mw );
  list->addColumn( i18n("Name") );
  list->addColumn( i18n("URL") );
  KTextEditor::Document *currentDoc = mainWindow->viewManager()->activeView()->document();
  uint n = KateDocManager::self()->documents();
  uint i = 0;
  Q3CheckListItem *item;
  while ( i < n ) {
    KTextEditor::Document *doc = KateDocManager::self()->document( i );
    if ( doc ) {
      item = new KateDocCheckItem( list, doc->documentName(), doc );
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

Q3PtrList<KTextEditor::Document> KateMailDialog::selectedDocs()
{
  Q3PtrList<KTextEditor::Document> l;
  Q3ListViewItem *item = list->firstChild();
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
#include "katemailfilesdialog.moc"
