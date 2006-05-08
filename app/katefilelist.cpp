/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//BEGIN Includes
#include "katefilelist.h"
#include "katefilelist.moc"

#include "katedocmanager.h"
#include "kateviewmanager.h"
#include "katemainwindow.h"

#include <qapplication.h>
#include <qpainter.h>
#include <QMenu>
#include <q3header.h>
#include <qcolor.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <q3groupbox.h>
#include <qlabel.h>
//Added by qt3to4:
#include <QPixmap>
#include <QGridLayout>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QMouseEvent>

#include <kiconloader.h>
#include <kconfig.h>
#include <klocale.h>
#include <kglobalsettings.h>
#include <kpassivepopup.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kstringhandler.h>
#include <kcolorbutton.h>
#include <kdialog.h>
#include <kxmlguifactory.h>
#include <kglobal.h>
#include <kcombobox.h>
#include <kselectaction.h>
#include <kstdaction.h>
//END Includes

//BEGIN ToolTip
/*
class ToolTip : public QToolTip
{
  public:
    ToolTip( QWidget *parent, KateFileList *lv )
      : QToolTip(),
    m_listView( lv )
    {
    }
    virtual ~ToolTip() {};

    void maybeTip( const QPoint &pos )
    {
      Q3ListViewItem *i = m_listView->itemAt( pos );
      if ( ! i ) return;

      KateFileListItem *item = ((KateFileListItem*)i);
      if ( ! item ) return;

      tip( m_listView->itemRect( i ), m_listView->tooltip( item, 0 ) );

    }

  private:
    KateFileList *m_listView;
};*/

//END ToolTip

//BEGIN KateFileList
KateFileList::KateFileList (KateMainWindow *main,
                            KateViewManager *_viewManager,
                            QWidget * parent)
    :  K3ListView (parent)
    , m_sort( KateFileList::sortByID )
{
  m_main = main;
  //m_tooltip = new ToolTip( viewport(), this );

  // default colors
  m_viewShade = QColor( 51, 204, 255 );
  m_editShade = QColor( 255, 102, 153 );
  m_enableBgShading = false;

  setFocusPolicy ( Qt::NoFocus  );

  viewManager = _viewManager;

  header()->hide();
  addColumn("Document Name");

  setSelectionMode( Q3ListView::Single );
  setSorting( 0, true );
  setShowToolTips( false );

  setupActions ();

  for (uint i = 0; i < KateDocManager::self()->documents(); i++)
  {
    slotDocumentCreated (KateDocManager::self()->document(i));
    slotModChanged (KateDocManager::self()->document(i));
  }

  connect(KateDocManager::self(),SIGNAL(documentCreated(KTextEditor::Document *)),
	  this,SLOT(slotDocumentCreated(KTextEditor::Document *)));
  connect(KateDocManager::self(),SIGNAL(documentDeleted(KTextEditor::Document *)),
	  this,SLOT(slotDocumentDeleted(KTextEditor::Document *)));

  // don't Honour KDE single/double click setting, this files are already open,
  // no need for hassle of considering double-click
  connect(this,SIGNAL(selectionChanged(Q3ListViewItem *)),
	  this,SLOT(slotActivateView(Q3ListViewItem *)));
  connect(viewManager,SIGNAL(viewChanged()), this,SLOT(slotViewChanged()));
  connect(this,SIGNAL(contextMenuRequested( Q3ListViewItem *, const QPoint &, int )),
	  this,SLOT(slotMenu ( Q3ListViewItem *, const QPoint &, int )));
}

KateFileList::~KateFileList ()
{
  //delete m_tooltip;
}

void KateFileList::setupActions ()
{
  windowNext = KStdAction::back(this, SLOT(slotPrevDocument()), m_main->actionCollection());
  windowPrev = KStdAction::forward(this, SLOT(slotNextDocument()), m_main->actionCollection());
  sortAction = new KSelectAction( i18n("Sort &By"), m_main->actionCollection(), "filelist_sortby"  );
  QStringList l;
  l << i18n("Opening Order") << i18n("Document Name") << i18n("URL");
  sortAction->setItems( l );
  connect( sortAction, SIGNAL(activated(int)), this, SLOT(setSortType(int)) );
}

void KateFileList::updateActions ()
{
  windowNext->setEnabled(KateDocManager::self()->documents()  > 1);
  windowPrev->setEnabled(KateDocManager::self()->documents()  > 1);
}

void KateFileList::keyPressEvent(QKeyEvent *e) {
  if ( ( e->key() == Qt::Key_Return ) || ( e->key() == Qt::Key_Enter ) )
  {
    e->accept();
    slotActivateView( currentItem() );
  }
  else
  {
    K3ListView::keyPressEvent(e);
  }
}

// Protect single mode selection: don't let them
// leftclick outside items.
// ### if we get to accept keyboard navigation, set focus before
// returning
void KateFileList::contentsMousePressEvent( QMouseEvent *e )
{
  if ( ! itemAt( contentsToViewport( e->pos() ) ) )
  return;

  K3ListView::contentsMousePressEvent( e );
}

void KateFileList::resizeEvent( QResizeEvent *e )
{
  K3ListView::resizeEvent( e );

  // ### We may want to actually calculate the widest field,
  // since it's not automatically scrinked. If I add support for
  // tree or marks, the changes of the required width will vary
  // a lot with opening/closing of files and display changes for
  // the mark branches.
  int w = viewport()->width();
  if ( columnWidth( 0 ) < w )
    setColumnWidth( 0, w );
}

void KateFileList::slotNextDocument()
{
  if ( ! currentItem() || childCount() == 0 )
    return;

  // ### more checking once more item types are added

  if ( currentItem()->nextSibling() )
    viewManager->activateView( ((KateFileListItem*)currentItem()->nextSibling())->document() );
  else
    viewManager->activateView( ((KateFileListItem *)firstChild())->document() );
}

void KateFileList::slotPrevDocument()
{
  if ( ! currentItem() || childCount() == 0 )
    return;

  // ### more checking once more item types are added

  if ( currentItem()->itemAbove() )
    viewManager->activateView( ((KateFileListItem*)currentItem()->itemAbove())->document() );
  else
    viewManager->activateView( ((KateFileListItem *)lastItem())->document() );
}

void KateFileList::slotDocumentCreated (KTextEditor::Document *doc)
{
  new KateFileListItem( this, doc/*, doc->documentNumber()*/ );
  connect(doc,SIGNAL(modifiedChanged(KTextEditor::Document *)),this,SLOT(slotModChanged(KTextEditor::Document *)));
  connect(doc,SIGNAL(documentNameChanged(KTextEditor::Document *)),this,SLOT(slotNameChanged(KTextEditor::Document *)));
  connect(doc,SIGNAL(modifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)),this,SLOT(slotModifiedOnDisc(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)));

  sort();
  updateActions ();
}

void KateFileList::slotDocumentDeleted (KTextEditor::Document *doc)
{
  Q3ListViewItem * item = firstChild();
  while( item ) {
    if ( ((KateFileListItem *)item)->document() == doc )
    {
      removeItem( item );

      break;
    }
    item = item->nextSibling();
  }

  updateActions ();
}

void KateFileList::slotActivateView( Q3ListViewItem *item )
{
  if ( ! item || item->rtti() != RTTI_KateFileListItem )
    return;

  viewManager->activateView( ((KateFileListItem *)item)->document() );
}

void KateFileList::slotModChanged (KTextEditor::Document *doc)
{
  Q3ListViewItem * item = firstChild();

  if (!doc || !item) return;

  while( item )
  {
    if ( ((KateFileListItem *)item)->document() == doc )
      break;

    item = item->nextSibling();
  }

  if ( ((KateFileListItem *)item)->document()->isModified() )
  {
    m_editHistory.removeAll( (KateFileListItem *)item );
    m_editHistory.prepend( (KateFileListItem *)item );

    for ( int i=0; i <  m_editHistory.count(); i++ )
    {
      m_editHistory.at( i )->setEditHistPos( i+1 );
      repaintItem(  m_editHistory.at( i ) );
    }
  }
  else
    repaintItem( item );
}

void KateFileList::slotModifiedOnDisc (KTextEditor::Document *doc, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason)
{
  Q_UNUSED(reason)

  slotModChanged( doc );
}

void KateFileList::slotNameChanged (KTextEditor::Document *doc)
{
  if (!doc) return;

  // ### using nextSibling to *only* look at toplevel items.
  // child items could be marks for example
  Q3ListViewItem * item = firstChild();
  while( item ) {
    if ( ((KateFileListItem*)item)->document() == doc )
    {
      item->setText( 0, doc->documentName() );
      repaintItem( item );
      break;
    }
    item = item->nextSibling();
  }
  updateSort();
}

void KateFileList::slotViewChanged ()
{
  if (!viewManager->activeView()) return;

  KTextEditor::View *view = viewManager->activeView();
  KTextEditor::Document *dn = view->document();

  Q3ListViewItem * i = firstChild();
  while( i ) {
    if ( ((KateFileListItem *)i)->document() == dn )
    {
      break;
    }
    i = i->nextSibling();
  }

  if ( ! i )
    return;

  KateFileListItem *item = (KateFileListItem*)i;
  setCurrentItem( item );

  // ### During load of file lists, all the loaded views gets active.
  // Do something to avoid shading them -- maybe not creating views, just
  // open the documents???


//   int p = 0;
//   if (  m_viewHistory.count() )
//   {
//     int p =  m_viewHistory.findRef( item ); // only repaint items that needs it
//   }

  m_viewHistory.removeAll( item );
  m_viewHistory.prepend( item );

  for ( int i=0; i <  m_viewHistory.count(); i++ )
  {
    m_viewHistory.at( i )->setViewHistPos( i+1 );
    repaintItem(  m_viewHistory.at( i ) );
  }

}

void KateFileList::slotMenu ( Q3ListViewItem *item, const QPoint &p, int /*col*/ )
{
  if (!item)
    return;

  QMenu *menu = (QMenu*) ((viewManager->mainWindow())->factory()->container("filelist_popup", viewManager->mainWindow()));

  if (menu)
    menu->exec(p);
}

QString KateFileList::tooltip( Q3ListViewItem *item, int ) const
{
  KateFileListItem *i = ((KateFileListItem*)item);
  if ( ! i ) return QString();

  QString str;
  const KateDocumentInfo *info = KateDocManager::self()->documentInfo(i->document());

  if (info && info->modifiedOnDisc)
  {
    if (info->modifiedOnDiscReason == 1)
      str += i18n("<b>This file was changed (modified) on disk by another program.</b><br />");
    else if (info->modifiedOnDiscReason == 2)
      str += i18n("<b>This file was changed (created) on disk by another program.</b><br />");
    else if (info->modifiedOnDiscReason == 3)
      str += i18n("<b>This file was changed (deleted) on disk by another program.</b><br />");
  }

  str += i->document()->url().prettyURL();
  return str;
}


void KateFileList::setSortType (int s)
{
  m_sort = s;
  updateSort ();
}

void KateFileList::updateSort ()
{
  sort ();
}

void KateFileList::readConfig( KConfig *config, const QString &group )
{
  QString oldgroup = config->group();
  config->setGroup( group );

  setSortType( config->readEntry( "Sort Type", int(sortByID) ) );
  m_viewShade = config->readEntry( "View Shade", m_viewShade );
  m_editShade = config->readEntry( "Edit Shade", m_editShade );
  m_enableBgShading = config->readEntry( "Shading Enabled", m_enableBgShading );

  sortAction->setCurrentItem( sortType() );

  config->setGroup( oldgroup );
}

void KateFileList::writeConfig( KConfig *config, const QString &group )
{
  QString oldgroup = config->group();
  config->setGroup( group );

  config->writeEntry( "Sort Type", m_sort );
  config->writeEntry( "View Shade", m_viewShade );
  config->writeEntry( "Edit Shade", m_editShade );
  config->writeEntry( "Shading Enabled", m_enableBgShading );

  config->setGroup( oldgroup );
}

void KateFileList::takeItem( Q3ListViewItem *item )
{
  if ( item->rtti() == RTTI_KateFileListItem )
  {
    m_editHistory.removeAll( (KateFileListItem*)item );
    m_viewHistory.removeAll( (KateFileListItem*)item );
  }
  Q3ListView::takeItem( item );
}
//END KateFileList

//BEGIN KateFileListItem
KateFileListItem::KateFileListItem( Q3ListView* lv,
				    KTextEditor::Document *_doc )
  : Q3ListViewItem( lv, _doc->documentName() ),
    doc( _doc ),
    m_viewhistpos( 0 ),
    m_edithistpos( 0 )
{
}

KateFileListItem::~KateFileListItem()
{
}

const QPixmap *KateFileListItem::pixmap ( int column ) const
{
  if ( column == 0) {
    static QPixmap noPm = SmallIcon ("null");
    static QPixmap modPm = SmallIcon("modified");
    static QPixmap discPm = SmallIcon("modonhd");
    static QPixmap modmodPm = SmallIcon("modmod");

    const KateDocumentInfo *info = KateDocManager::self()->documentInfo(doc);

    if (info && info->modifiedOnDisc)
      return doc->isModified() ? &modmodPm : &discPm;
    else
      return doc->isModified() ? &modPm : &noPm;
  }

  return 0;
}

void KateFileListItem::paintCell( QPainter *painter, const QColorGroup & cg, int column, int width, int align )
{
  KateFileList *fl = (KateFileList*)listView();
  if ( ! fl ) return;

  if ( column == 0 )
  {
    QColorGroup cgNew = cg;

    // replace the base color with a different shading if necessary...
    if ( fl->shadingEnabled() && m_viewhistpos > 1 )
    {
      QColor b = cg.color(QPalette::Base);

      QColor shade = fl->viewShade();
      QColor eshade = fl->editShade();
      int hc = fl->histCount();
      // If this file is in the edit history, blend in the eshade
      // color. The blend is weighted by the position in the editing history
      if ( fl->shadingEnabled() && m_edithistpos > 0 )
      {
        int ec = fl->editHistCount();
        int v = hc-m_viewhistpos;
        int e = ec-m_edithistpos+1;
        e = e*e;
        int n = qMax(v + e, 1);
        shade.setRgb(
            ((shade.red()*v) + (eshade.red()*e))/n,
            ((shade.green()*v) + (eshade.green()*e))/n,
            ((shade.blue()*v) + (eshade.blue()*e))/n
                    );
      }
      // blend in the shade color.
      // max transperancy < .5, latest is most colored.
      float t = (0.5/hc)*(hc-m_viewhistpos+1);
      b.setRgb(
          (int)((b.red()*(1-t)) + (shade.red()*t)),
          (int)((b.green()*(1-t)) + (shade.green()*t)),
          (int)((b.blue()*(1-t)) + (shade.blue()*t))
              );

      cgNew.setColor(QPalette::Base, b);
    }

    Q3ListViewItem::paintCell( painter, cgNew, column, width, align );
  }
  else
    Q3ListViewItem::paintCell( painter, cg, column, width, align );
}

int KateFileListItem::compare ( Q3ListViewItem * i, int col, bool ascending ) const
{
  if ( i->rtti() == RTTI_KateFileListItem )
  {
    switch( ((KateFileList*)listView())->sortType() )
    {
      case KateFileList::sortByID:
      {
        int d = KateDocManager::self()->findDocument (doc)
             - KateDocManager::self()->findDocument (((KateFileListItem*)i)->document());
        return ascending ? d : -d;
        break;
      }
      case KateFileList::sortByURL:
        return doc->url().prettyURL().compare( ((KateFileListItem*)i)->document()->url().prettyURL() );
        break;
      default:
        return Q3ListViewItem::compare( i, col, ascending );
    }
  }
  return 0;
}
//END KateFileListItem

//BEGIN KFLConfigPage
KFLConfigPage::KFLConfigPage( QWidget* parent, KateFileList *fl )
  :  KTextEditor::ConfigPage( parent ),
    m_filelist( fl ),
    m_changed( false )
{
  QVBoxLayout *lo1 = new QVBoxLayout( this );
  int spacing = KDialog::spacingHint();
  lo1->setSpacing( spacing );

  Q3GroupBox *gb = new Q3GroupBox( 1, Qt::Horizontal, i18n("Background Shading"), this );
  lo1->addWidget( gb );

  QWidget *g = new QWidget( gb );
  QGridLayout *lo = new QGridLayout( g );
  lo->setSpacing( KDialog::spacingHint() );
  cbEnableShading = new QCheckBox( i18n("&Enable background shading"), g );
  lo->addWidget( cbEnableShading, 1, 1, 0, 1 );

  kcbViewShade = new KColorButton( g );
  lViewShade = new QLabel( g );
  lViewShade->setBuddy( kcbViewShade );
  lViewShade->setText( i18n("&Viewed documents' shade:") );
  lo->addWidget( lViewShade, 2, 0 );
  lo->addWidget( kcbViewShade, 2, 1 );

  kcbEditShade = new KColorButton( g );
  lEditShade = new QLabel( g );
  lEditShade->setBuddy( kcbEditShade );
  lEditShade->setText( i18n("&Modified documents' shade:") );
  lo->addWidget( lEditShade, 3, 0 );
  lo->addWidget( kcbEditShade, 3, 1 );

  // sorting
  KHBox *hbSorting = new KHBox( this );
  lo1->addWidget( hbSorting );
  lSort = new QLabel( i18n("&Sort by:"), hbSorting );
  cmbSort = new KComboBox( hbSorting );
  lSort->setBuddy( cmbSort );
  QStringList l;
  l << i18n("Opening Order") << i18n("Document Name") << i18n("URL");
  cmbSort->addItems( l );

  lo1->insertStretch( -1, 10 );

  cbEnableShading->setWhatsThis(i18n(
      "When background shading is enabled, documents that have been viewed "
      "or edited within the current session will have a shaded background. "
      "The most recent documents have the strongest shade.") );
  kcbViewShade->setWhatsThis(i18n(
      "Set the color for shading viewed documents.") );
  kcbEditShade->setWhatsThis(i18n(
      "Set the color for modified documents. This color is blended into "
      "the color for viewed files. The most recently edited documents get "
      "most of this color.") );

  cmbSort->setWhatsThis(i18n(
      "Set the sorting method for the documents.") );

  reset();

  slotEnableChanged();
  connect( cbEnableShading, SIGNAL(toggled(bool)), this, SLOT(slotMyChanged()) );
  connect( cbEnableShading, SIGNAL(toggled(bool)), this, SLOT(slotEnableChanged()) );
  connect( kcbViewShade, SIGNAL(changed(const QColor&)), this, SLOT(slotMyChanged()) );
  connect( kcbEditShade, SIGNAL(changed(const QColor&)), this, SLOT(slotMyChanged()) );
  connect( cmbSort, SIGNAL(activated(int)), this, SLOT(slotMyChanged()) );
}

void KFLConfigPage::apply()
{
  if ( ! m_changed )
    return;
  m_changed = false;

  // Change settings in the filelist
  m_filelist->m_viewShade = kcbViewShade->color();
  m_filelist->m_editShade = kcbEditShade->color();
  m_filelist->m_enableBgShading = cbEnableShading->isChecked();
  m_filelist->setSortType( cmbSort->currentIndex() );
  // repaint the affected items
  m_filelist->triggerUpdate();
}

void KFLConfigPage::reset()
{
  // read in from config file
  KConfig *config = KGlobal::config();
  config->setGroup( "Filelist" );
  cbEnableShading->setChecked( config->readEntry("Shading Enabled", QVariant(&m_filelist->m_enableBgShading )).toBool() );
  kcbViewShade->setColor( config->readEntry("View Shade", m_filelist->m_viewShade ) );
  kcbEditShade->setColor( config->readEntry("Edit Shade", m_filelist->m_editShade ) );
  cmbSort->setCurrentIndex( m_filelist->sortType() );
  m_changed = false;
}

void KFLConfigPage::slotEnableChanged()
{
  kcbViewShade->setEnabled( cbEnableShading->isChecked() );
  kcbEditShade->setEnabled( cbEnableShading->isChecked() );
  lViewShade->setEnabled( cbEnableShading->isChecked() );
  lEditShade->setEnabled( cbEnableShading->isChecked() );
}

void KFLConfigPage::slotMyChanged()
{
  emit changed();
  m_changed = true;
}

//END KFLConfigPage


// kate: space-indent on; indent-width 2; replace-tabs on;
