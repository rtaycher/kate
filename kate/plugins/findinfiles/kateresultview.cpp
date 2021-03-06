/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001, 2004 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007-2008 Dominik Haumann <dhaumann kde org>
   Copyright (C) 2007 Flavio Castelli <flavio.castelli@gmail.com>
   Copyright (C) 2008 Eduardo Robles Elvira <edulix@gmail.com>

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

#include "kateresultview.h"
#include "katefindinfiles.h"
#include "kategrepthread.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <kdebug.h>

#include <QClipboard>
#include <QRegExp>
#include <QKeyEvent>
#include <QtGui/QMenu>

#include <kacceleratormanager.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>

KateResultView::KateResultView(Kate::MainWindow *mw, KateFindInFilesView *view)
    : QWidget()
    , m_mw (mw)
    , m_toolView(0)
    , m_view(view)
    , m_grepThread(new KateGrepThread(this))
{
  m_id = view->freeId();
  QString name = i18n("Find in Files");
  if (m_id > 1)
    name = i18n("Find in Files %1", m_id);
  m_toolView = mw->createToolView(QString("katefindinfilesplugin_%1").arg(m_id),
                 Kate::MainWindow::Bottom, SmallIcon("edit-find"), name);

  setupUi(this);
  setParent(m_toolView);

  btnCancel->setIcon(KIcon("process-stop"));

  btnRemove->setGuiItem(KStandardGuiItem::remove());
  btnRemove->setIcon(KIcon("tab-close"));

  btnRefine->setGuiItem(KStandardGuiItem::find());
  btnRefine->setText(i18n("Refine Search..."));

  QMenu* openMenu = new QMenu(btnOpen);
  btnOpen->setMenu(openMenu);
  btnOpen->setGuiItem(KStandardGuiItem::open());
  btnOpen->setText(i18n("Open"));
  openMenu->addAction(i18n("Open All Files"), this, SLOT(openAllFound()));
  openMenu->addAction(i18n("Open Selected Files"), this, SLOT(openSelected()));


  // auto-accels
  KAcceleratorManager::manage(m_toolView);

  twResults->installEventFilter(this);
  twResults->setWhatsThis(i18n("The results of the grep run are listed here. Select a\n"
                               "filename/line number combination and press Enter or doubleclick\n"
                               "on the item to show the respective line in the editor."));
  setFocusProxy(twResults);

  connect(twResults, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),
          this, SLOT(itemSelected(QTreeWidgetItem *, int)));

  connect(btnCancel, SIGNAL(clicked()), this, SLOT(killThread()));
  connect(btnRemove, SIGNAL(clicked()), this, SLOT(deleteToolview()));
  connect(btnRefine, SIGNAL(clicked()), this, SLOT(refineSearch()));
  connect(m_grepThread, SIGNAL(finished()), this, SLOT(searchFinished()));
  connect(m_grepThread, SIGNAL(foundMatch (const QString &, const QString &, const QList<int> &,
                     const QList<int> &, const QString &, const QStringList &)),
          this, SLOT(searchMatchFound(const QString &, const QString &,const QList<int> &,
                     const QList<int> &, const QString &, const QStringList &)),
          Qt::QueuedConnection);

  setStatusVisible(false);
}


KateResultView::~KateResultView()
{
  killThread ();
  delete m_grepThread;
  m_grepThread = 0;
}

void KateResultView::setStatusVisible(bool visible)
{
  progressBar->setVisible(visible);
  lblStatus->setVisible(visible);
  btnCancel->setVisible(visible);
}

QWidget* KateResultView::toolView()
{
  return m_toolView;
}

void KateResultView::makeVisible()
{
  m_mw->showToolView(m_toolView);
}

void KateResultView::startSearch(KateFindInFilesOptions options,
                                 const QList<QRegExp>& pattern,
                                 const QString& url,
                                 const QString& filter)
{
  killThread();
  setStatusVisible(true);
  twResults->clear();

  // save search options
  m_options = options;
  m_lastPatterns = pattern;
  m_lastUrl = url;
  m_lastFilter = filter;

  m_grepThread->startSearch(pattern,
                            url,
                            filter.split(QRegExp("[,;]"), QString::SkipEmptyParts),
                            m_options.recursive(),
                            m_options.followDirectorySymlinks(),
                            m_options.includeHiddenFiles());
}


void KateResultView::killThread ()
{
  if (m_grepThread->isRunning()) {
    m_grepThread->cancel();
    m_grepThread->wait();
    layoutColumns();
  }
  setStatusVisible(false);
}

void KateResultView::itemSelected(QTreeWidgetItem *item, int)
{
  // get stuff
  const QString filename = item->data(0, Qt::UserRole).toString();
  const int linenumber = item->data(1, Qt::UserRole).toInt();
  const int column = item->data(2, Qt::UserRole).toInt();

  // open file (if needed, otherwise, this will activate only the right view...)
  KUrl fileURL;
  fileURL.setPath( filename );
  m_mw->openUrl( fileURL );

  // any view active?
  if ( !m_mw->activeView() )
    return;

  // do it ;)
  m_mw->activeView()->setCursorPosition( KTextEditor::Cursor (linenumber, column) );
  m_mw->activeView()->setFocus();
}

void KateResultView::searchFinished ()
{
  setStatusVisible(false);
  layoutColumns();
}

void KateResultView::searchMatchFound(const QString &filename, const QString &relname, const QList<int> &lines, const QList<int> &columns, const QString &/*basename*/, const QStringList &lineContent)
{
  QList<QTreeWidgetItem *> items;
  for (int i = 0; i < lines.size(); ++i)
  {
    QTreeWidgetItem* item = new QTreeWidgetItem();

    // visible text
    item->setText(0, relname);
    item->setText(1, QString::number (lines[i] + 1));
    item->setText(2, lineContent[i].trimmed());

    // used to read from when activating an item
    item->setData(0, Qt::UserRole, filename);
    item->setData(1, Qt::UserRole, lines[i]);
    item->setData(2, Qt::UserRole, columns[i]);

    // add tooltips in all columns
    item->setData(0, Qt::ToolTipRole, filename);
    item->setData(1, Qt::ToolTipRole, filename);
    item->setData(2, Qt::ToolTipRole, filename);

    items.append (item);
  }

  twResults->addTopLevelItems (items);
}

void KateResultView::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_Escape)
  {
    m_mw->hideToolView(m_toolView);
    event->accept();
    return;
  }
  if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
  {
    if( !(twResults->selectedItems().empty()) )
    {
      itemSelected(twResults->currentItem(), 0);
      event->accept();
      return;
    }
  }
  QWidget::keyPressEvent(event);
}

void KateResultView::layoutColumns()
{
  twResults->resizeColumnToContents(0);
  twResults->resizeColumnToContents(1);
  twResults->resizeColumnToContents(2);
}

void KateResultView::deleteToolview()
{
  killThread();
  m_view->removeResultView(this);
  m_toolView->deleteLater();
}

void KateResultView::refineSearch()
{
  KateFindDialog* dlg = m_view->findDialog();
  dlg->setPattern(m_lastPatterns);
  dlg->setUrl(m_lastUrl);
  dlg->setFilter(m_lastFilter);
  dlg->setOptions(m_options);
  dlg->useResultView(m_id);
  dlg->show();
}

void KateResultView::openAllFound()
{
  QString lastFilename;
  QString filename;
  for (int i = 0; i < twResults->topLevelItemCount(); ++i )
  {
    QTreeWidgetItem *item = twResults->topLevelItem(i);
    filename = item->data(0, Qt::UserRole).toString();
    if ( filename != lastFilename )
    {
      itemSelected(item, 0);
      lastFilename = filename;
    }
  }
}

void KateResultView::openSelected()
{
  QString lastFilename;
  QString filename;
  foreach( QTreeWidgetItem *item, twResults->selectedItems() )
  {
    filename = item->data(0, Qt::UserRole).toString();
    if ( filename != lastFilename )
    {
      itemSelected(item, 0);
      lastFilename = filename;
    }
  }
}

int KateResultView::id() const
{
  return m_id;
}

bool KateResultView::eventFilter( QObject *o, QEvent *e )
{
  if ( e->type() == QEvent::KeyPress )
  {
    QKeyEvent *ke = static_cast<QKeyEvent*>(e);
    if (o == twResults && (ke == QKeySequence::Copy))
    {
      // user pressed ctrl+c -> copy full URL to the clipboard
      QModelIndex idx = twResults->model()->index(twResults->currentIndex().row(), 0);
      QVariant variant = twResults->model()->data(idx, Qt::UserRole);
      if (variant.type() == QVariant::String)
          QApplication::clipboard()->setText(variant.toString());
      e->accept();
      return true;
    }
  }

  return QWidget::eventFilter( o, e );
}

#include "kateresultview.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
