/* This file is part of the KDE libraries
   Copyright (C) 2005-2006 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>

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

#include "katecompletionwidget.h"

#include <QtGui/QBoxLayout>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QHeaderView>
#include <QtCore/QTimer>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>
#include <QtGui/QSizeGrip>
#include <QtGui/QPushButton>
#include <QtGui/QAbstractScrollArea>
#include <QtGui/QScrollBar>
#include <QtCore/QMutex>

#include <kicon.h>
#include <kdialog.h>

#include <ktexteditor/cursorfeedback.h>
#include <ktexteditor/codecompletionmodelcontrollerinterface.h>

#include "kateview.h"
#include "katesmartmanager.h"
#include "katerenderer.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "katesmartrange.h"
#include "kateedit.h"

#include "katecompletionmodel.h"
#include "katecompletiontree.h"
#include "katecompletionconfig.h"
#include "kateargumenthinttree.h"
#include "kateargumenthintmodel.h"

//#include "modeltest.h"

KTextEditor::CodeCompletionModelControllerInterface* modelController(KTextEditor::CodeCompletionModel *model)
{
  static KTextEditor::CodeCompletionModelControllerInterface defaultIf;
  KTextEditor::CodeCompletionModelControllerInterface* ret =
    qobject_cast<KTextEditor::CodeCompletionModelControllerInterface*>(model);
  if (!ret) {
    ret = &defaultIf;
  }
  return ret;
}


KateCompletionWidget::KateCompletionWidget(KateView* parent)
  : QFrame(parent, Qt::ToolTip)
  , m_presentationModel(new KateCompletionModel(this))
  , m_entryList(new KateCompletionTree(this))
  , m_argumentHintModel(new KateArgumentHintModel(this))
  , m_argumentHintTree(new KateArgumentHintTree(this))
  , m_automaticInvocationDelay(300)
  , m_filterInstalled(false)
  , m_configWidget(new KateCompletionConfig(m_presentationModel, view()))
  , m_lastInsertionByUser(false)
  , m_inCompletionList(false)
  , m_isSuspended(false)
  , m_dontShowArgumentHints(false)
  , m_needShow(false)
  , m_hadCompletionNavigation(false)
  , m_expandedAddedHeightBase(0)
  , m_expandingAddedHeight(0)
{
  connect(parent, SIGNAL(navigateAccept()), SLOT(navigateAccept()));
  connect(parent, SIGNAL(navigateBack()), SLOT(navigateBack()));
  connect(parent, SIGNAL(navigateDown()), SLOT(navigateDown()));
  connect(parent, SIGNAL(navigateLeft()), SLOT(navigateLeft()));
  connect(parent, SIGNAL(navigateRight()), SLOT(navigateRight()));
  connect(parent, SIGNAL(navigateUp()), SLOT(navigateUp()));

  setFrameStyle( QFrame::Box | QFrame::Plain );
  setLineWidth( 1 );
  //setWindowOpacity(0.8);

  m_entryList->setModel(m_presentationModel);
  m_entryList->setColumnWidth(0, 0); //These will be determined automatically in KateCompletionTree::resizeColumns
  m_entryList->setColumnWidth(1, 0);
  m_entryList->setColumnWidth(2, 0);

  m_argumentHintTree->setParent(0, Qt::ToolTip);
  m_argumentHintTree->setModel(m_argumentHintModel);

  connect(m_entryList->verticalScrollBar(), SIGNAL(valueChanged(int)), m_presentationModel, SLOT(placeExpandingWidgets()));
  connect(m_argumentHintTree->verticalScrollBar(), SIGNAL(valueChanged(int)), m_argumentHintModel, SLOT(placeExpandingWidgets()));
  connect(view(), SIGNAL(focusOut(KTextEditor::View*)), this, SLOT(viewFocusOut()));

  m_automaticInvocationTimer = new QTimer(this);
  m_automaticInvocationTimer->setSingleShot(true);
  connect(m_automaticInvocationTimer, SIGNAL(timeout()), this, SLOT(automaticInvocation()));

//   QVBoxLayout* vl = new QVBoxLayout(this);
//   vl->addWidget(m_entryList);
//   vl->setMargin(0);

  // Keep branches expanded
  connect(m_presentationModel, SIGNAL(modelReset()), this, SLOT(modelReset()));
  connect(m_presentationModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), SLOT(rowsInserted(const QModelIndex&, int, int)));
  connect(m_argumentHintModel, SIGNAL(contentStateChanged(bool)), this, SLOT(argumentHintsChanged(bool)));

  // These must be queued connections so that we're not holding the smart lock when we ask for the model to update.
  connect(view(), SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), this, SLOT(cursorPositionChanged()), Qt::QueuedConnection);
  connect(view()->doc()->history(), SIGNAL(editDone(KateEditInfo*)), SLOT(editDone(KateEditInfo*)));
  connect(view(), SIGNAL(verticalScrollPositionChanged (KTextEditor::View*, const KTextEditor::Cursor&)), this, SLOT(updatePositionSlot()), Qt::QueuedConnection);

  // This is a non-focus widget, it is passed keyboard input from the view

  //We need to do this, because else the focus goes to nirvana without any control when the completion-widget is clicked.
  setFocusPolicy(Qt::ClickFocus);
  m_argumentHintTree->setFocusPolicy(Qt::ClickFocus);

  foreach (QWidget* childWidget, findChildren<QWidget*>())
    childWidget->setFocusPolicy(Qt::NoFocus);
  
  //Position the entry-list so a frame can be drawn around it
  m_entryList->move(frameWidth(), frameWidth());
}

KateCompletionWidget::~KateCompletionWidget() {
}

void KateCompletionWidget::viewFocusOut() {
  abortCompletion();
}

void KateCompletionWidget::modelContentChanged() {
  if(m_completionRanges.isEmpty()) {
    kDebug( 13035 ) << "content changed, but no completion active";
    hide();
    return;
  }
  
  int realItemCount = 0;
  foreach (KTextEditor::CodeCompletionModel* model, m_presentationModel->completionModels())
    realItemCount += model->rowCount();
  if( !m_isSuspended && (!!isHidden() || m_needShow) && realItemCount != 0 ) {
    m_needShow = false;
    updateAndShow();
  }

  if(m_presentationModel->rowCount(QModelIndex()) == 0 && m_argumentHintModel->rowCount(QModelIndex()) == 0) {
    kDebug( 13035 ) << "hiding because no content";
    hide();
    return;
  }

  //With each filtering items can be added or removed, so we have to reset the current index here so we always have a selected item
  m_entryList->setCurrentIndex(model()->index(0,0));
  if(!model()->indexIsItem(m_entryList->currentIndex())) {
    QModelIndex firstIndex = model()->index(0,0, m_entryList->currentIndex());
    m_entryList->setCurrentIndex(firstIndex);
    //m_entryList->scrollTo(firstIndex, QAbstractItemView::PositionAtTop);
  }
}

KateArgumentHintTree* KateCompletionWidget::argumentHintTree() const {
  return m_argumentHintTree;
}

KateArgumentHintModel* KateCompletionWidget::argumentHintModel() const {
  return m_argumentHintModel;
}

const KateCompletionModel* KateCompletionWidget::model() const {
  return m_presentationModel;
}

KateCompletionModel* KateCompletionWidget::model() {
  return m_presentationModel;
}

void KateCompletionWidget::rowsInserted(const QModelIndex& parent, int rowFrom, int rowEnd)
{
  m_entryList->setAnimated(false);
  if (!parent.isValid())
    for (int i = rowFrom; i <= rowEnd; ++i)
      m_entryList->expand(m_presentationModel->index(i, 0, parent));
}

KateView * KateCompletionWidget::view( ) const
{
  return static_cast<KateView*>(const_cast<QObject*>(parent()));
}

void KateCompletionWidget::argumentHintsChanged(bool hasContent)
{
  m_dontShowArgumentHints = !hasContent;

  if( m_dontShowArgumentHints )
    m_argumentHintTree->hide();
  else
    updateArgumentHintGeometry();
}

void KateCompletionWidget::startCompletion(KTextEditor::CodeCompletionModel::InvocationType invocationType)
{
  startCompletion(KTextEditor::Range(KTextEditor::Cursor(-1, -1), KTextEditor::Cursor(-1, -1)), 0, invocationType);
}

void KateCompletionWidget::startCompletion(const KTextEditor::Range& word, KTextEditor::CodeCompletionModel* model, KTextEditor::CodeCompletionModel::InvocationType invocationType)
{
  m_isSuspended = false;
  m_inCompletionList = true; //Always start at the top of the completion-list
  m_needShow = true;

  disconnect(this->model(), SIGNAL(contentGeometryChanged()), this, SLOT(modelContentChanged()));

  m_dontShowArgumentHints = true;

  QList<KTextEditor::CodeCompletionModel*> models;
  if (model) {
    models << model;
  } else {
    models = m_sourceModels;
  }
  
  foreach(KTextEditor::CodeCompletionModel* model, m_completionRanges.keys())
    if(!models.contains(model))
      models << model;

  if (!m_filterInstalled) {
    if (!QApplication::activeWindow()) {
      kWarning(13035) << "No active window to install event filter on!!";
      return;
    }
    // Enable the cc box to move when the editor window is moved
    QApplication::activeWindow()->installEventFilter(this);
    m_filterInstalled = true;
  }

  m_presentationModel->setCompletionModels(models);

  foreach (KTextEditor::CodeCompletionModel* model, models) {
    KTextEditor::Range range;
    if (word.isValid()) {
      range = word;
    } else {
      range = modelController(model)->completionRange(view(), view()->cursorPosition());
    }
    if(!range.isValid()) {
      if(m_completionRanges.contains(model)) {
        KTextEditor::SmartRange *oldRange = m_completionRanges[model];
        m_completionRanges.remove(model);
        delete oldRange;
      }
      m_presentationModel->removeCompletionModel(model);
      continue;
    }
    if(m_completionRanges.contains(model)) {
      if(*m_completionRanges[model] == range) {
        continue; //Leave it running as it is
      }
      else { // delete the range that was used previously
        KTextEditor::SmartRange *oldRange = m_completionRanges[model];
        m_completionRanges.remove(model);
        delete oldRange;
      }
    }
    model->completionInvoked(view(), range, invocationType);
    m_completionRanges[model] = view()->doc()->smartManager()->newSmartRange(range);
    m_completionRanges[model]->setInsertBehavior(KTextEditor::SmartRange::ExpandRight | KTextEditor::SmartRange::ExpandLeft);
    if(!m_completionRanges[model]->isValid()) {
      kWarning(13035) << "Could not construct valid smart-range from" << range << "instead got" << *m_completionRanges[model];
      abortCompletion();
      return;
    }
    connect(m_completionRanges[model]->smartStart().notifier(), SIGNAL(characterDeleted(KTextEditor::SmartCursor*, bool)),
              SLOT(startCharacterDeleted(KTextEditor::SmartCursor*, bool)));
  }

  cursorPositionChanged();

  if (!m_completionRanges.isEmpty()) {
    connect(this->model(), SIGNAL(contentGeometryChanged()), this, SLOT(modelContentChanged()));
    //Now that all models have been notified, check whether the widget should be displayed instantly
    modelContentChanged();
  }
  else {
    abortCompletion();
  }
}

void KateCompletionWidget::updateAndShow()
{
  setUpdatesEnabled(false);

  modelReset();

    m_argumentHintModel->buildRows();
    if( m_argumentHintModel->rowCount(QModelIndex()) != 0 )
      argumentHintsChanged(true);
//   }

  //We do both actions twice here so they are stable, because they influence each other:
  //updatePosition updates the height, resizeColumns needs the correct height to decide over
  //how many rows it computs the column-width
  updatePosition(true);
  m_entryList->resizeColumns(false, true, true);
  updatePosition(true);
  m_entryList->resizeColumns(false, true, true);
  
  setUpdatesEnabled(true);
  if (m_presentationModel->rowCount() || m_argumentHintModel->rowCount(QModelIndex()))
    show();
}

void KateCompletionWidget::updatePositionSlot()
{
  updatePosition();
}

bool KateCompletionWidget::updatePosition(bool force)
{
  if (!force && !isCompletionActive())
    return false;

  if (!completionRange()) {
    return false;
  }
  QPoint cursorPosition = view()->cursorToCoordinate(completionRange()->start());
  if (cursorPosition == QPoint(-1,-1)) {
    // Start of completion range is now off-screen -> abort
    abortCompletion();
    return false;
  }

  QPoint p = view()->mapToGlobal( cursorPosition );
  int x = p.x() - m_entryList->columnViewportPosition(m_presentationModel->translateColumn(KTextEditor::CodeCompletionModel::Name)) - 2;
  int y = p.y();
  //We do not need to move the widget up, because updateHeight will resize the widget to fit the screen
/*  if ( y + height() + view()->renderer()->config()->fontMetrics().height() > QApplication::desktop()->screenGeometry(this).bottom() )
    y -= height();
  else*/
  y += view()->renderer()->config()->fontMetrics().height();

  bool borderHit = false;
  
  if (x + width() > QApplication::desktop()->screenGeometry(view()).right()) {
    x = QApplication::desktop()->screenGeometry(view()).right() - width();
    borderHit = true;
  }

  if( x < QApplication::desktop()->screenGeometry(view()).left() ) {
    x = QApplication::desktop()->screenGeometry(view()).left();
    borderHit = true;
  }

  move( QPoint(x,y) );

  updateHeight();

  updateArgumentHintGeometry();
  
//   kDebug() << "updated to" << geometry() << m_entryList->geometry() << borderHit;
  
  return borderHit;
}

void KateCompletionWidget::updateArgumentHintGeometry()
{
  if( !m_dontShowArgumentHints ) {
    //Now place the argument-hint widget
    QRect geom = m_argumentHintTree->geometry();
    geom.moveTo(pos());
    geom.setWidth(width());
    geom.moveBottom(pos().y() - view()->renderer()->config()->fontMetrics().height()*2);
    m_argumentHintTree->updateGeometry(geom);
  }
}

void KateCompletionWidget::updateHeight()
{
  QRect geom = geometry();

  int baseHeight = geom.height() - m_expandingAddedHeight;

  if( m_expandedAddedHeightBase != baseHeight && m_expandedAddedHeightBase - baseHeight > -2 && m_expandedAddedHeightBase - baseHeight < 2  )
  {
    //Re-use the stored base-height if it only slightly differs from the current one.
    //Reason: Qt seems to apply slightly wrong sizes when the completion-widget is moved out of the screen at the bottom,
    //        which completely breaks this algorithm. Solution: re-use the old base-size if it only slightly differs from the computed one.
    baseHeight = m_expandedAddedHeightBase;
  }

  if( baseHeight < 300 ) {
    baseHeight = 300; //Here we enforce a minimum desirable height
    m_expandingAddedHeight = 0;
//     kDebug( 13035 ) << "Resetting baseHeight and m_expandingAddedHeight";
  }

  int newExpandingAddedHeight = 0;

//   kDebug( 13035 ) << "baseHeight: " << baseHeight;

  newExpandingAddedHeight = model()->expandingWidgetsHeight();

//   kDebug( 13035 ) << "new newExpandingAddedHeight: " << newExpandingAddedHeight;

  int screenBottom = QApplication::desktop()->screenGeometry(view()).bottom();

  int bottomPosition = baseHeight + newExpandingAddedHeight + geometry().top();
//  int targetHeight = baseHeight + newExpandingAddedHeight;
//   kDebug( 13035 ) << "targetHeight: " << targetHeight;

//   kDebug( 13035 ) << "screen-bottom: " << screenBottom << " bottomPosition: " << bottomPosition;

  if( bottomPosition > screenBottom-50 ) {
    newExpandingAddedHeight -= bottomPosition - (screenBottom-50);
//     kDebug( 13035 ) << "Too high, moved bottomPosition to: " << baseHeight + newExpandingAddedHeight + geometry().top() << " changed newExpandingAddedHeight to " << newExpandingAddedHeight;
  }

  int finalHeight = baseHeight+newExpandingAddedHeight;
//   kDebug( 13035 ) << "finalHeight: " << finalHeight;
  if( finalHeight < 50 ) {
    m_entryList->resize(m_entryList->width(), height() - 2*frameWidth());
    return;
  }

  m_expandingAddedHeight = baseHeight;
  m_expandedAddedHeightBase = geometry().height();

  geom.setHeight(finalHeight);

  setGeometry(geom);
    m_entryList->resize(m_entryList->width(), height() - 2*frameWidth());
}


void KateCompletionWidget::cursorPositionChanged( )
{
  if (m_completionRanges.isEmpty())
    return;

  KTextEditor::Cursor cursor = view()->cursorPosition();

  //Check the models and eventuall abort some
  QMutableMapIterator<KTextEditor::CodeCompletionModel*, KateSmartRange*> i(m_completionRanges);
  while (i.hasNext()) {
      i.next();
      KTextEditor::CodeCompletionModel *model = i.key();
      KateSmartRange* range = i.value();
      modelController(model)->updateCompletionRange(view(), *range);
      QString currentCompletion = modelController(model)->filterString(view(), *range, view()->cursorPosition());
      bool abort = modelController(model)->shouldAbortCompletion(view(), *range, currentCompletion);

      if (abort) {
        if (m_completionRanges.count() == 1) {
          //last model - abort whole completion
          abortCompletion();
          return;
        } else {
          modelController(model)->aborted(view());
          i.remove();
          delete range;
          m_presentationModel->removeCompletionModel(model);
        }
      } else {
        m_presentationModel->setCurrentCompletion(model, currentCompletion);
      }
  }

  m_entryList->scheduleUpdate();
}

bool KateCompletionWidget::isCompletionActive( ) const
{
  return !m_completionRanges.isEmpty() && !isHidden() && isVisible();
}

void KateCompletionWidget::abortCompletion( )
{
  kDebug(13035) ;

  m_isSuspended = false;

  bool wasActive = isCompletionActive();

  clear();

  if(!isHidden())
    hide();

  if (wasActive)
    view()->sendCompletionAborted();
}

void KateCompletionWidget::clear() {
  m_presentationModel->clearCompletionModels();
  m_argumentHintTree->clearCompletion();
  m_argumentHintModel->clear();
  
  foreach(KTextEditor::CodeCompletionModel* model, m_completionRanges.keys())
    modelController(model)->aborted(view());
  
  qDeleteAll(m_completionRanges);
  m_completionRanges.clear();
}

bool KateCompletionWidget::navigateAccept() {
  m_hadCompletionNavigation = true;
  
  if(currentEmbeddedWidget())
    QMetaObject::invokeMethod(currentEmbeddedWidget(), "embeddedWidgetAccept");
  
  QModelIndex index = selectedIndex();
  if( index.isValid() ) {
    view()->doc()->smartMutex()->unlock();
    index.data(KTextEditor::CodeCompletionModel::AccessibilityAccept);
    view()->doc()->smartMutex()->lock();
    return true;
  }
  return false;
}

void KateCompletionWidget::execute()
{
  kDebug(13035) ;

  if (!isCompletionActive())
    return;

  QModelIndex index = selectedIndex();
  
  if (!index.isValid())
    return abortCompletion();

  QModelIndex toExecute;
  
  KTextEditor::Cursor oldPos = view()->cursorPosition();
  
  if(index.model() == m_presentationModel)
    toExecute = m_presentationModel->mapToSource(index);
  else
    toExecute = m_argumentHintModel->mapToSource(index);

  if (!toExecute.isValid()) {
    kWarning() << k_funcinfo << "Could not map index" << m_entryList->selectionModel()->currentIndex() << "to source index.";
    return abortCompletion();
  }

  // encapsulate all editing as being from the code completion, and undo-able in one step.
  view()->doc()->editStart(true, Kate::CodeCompletionEdit);

  KTextEditor::CodeCompletionModel* model = static_cast<KTextEditor::CodeCompletionModel*>(const_cast<QAbstractItemModel*>(toExecute.model()));
  Q_ASSERT(model);

  KTextEditor::CodeCompletionModel2* model2 = qobject_cast<KTextEditor::CodeCompletionModel2*>(model);

  Q_ASSERT(m_completionRanges.contains(model));
  KTextEditor::Cursor start = m_completionRanges[model]->start();

  //editStart locks the smart-mutex, but it must not be locked when calling external functions,
  //else we may get deadlock-issues.
  view()->doc()->smartMutex()->unlock();
  
  if(model2)
    model2->executeCompletionItem2(view()->document(), *m_completionRanges[model], toExecute);
  else if(toExecute.parent().isValid())
    //The normale CodeCompletionInterface cannot handle feedback for hierarchical models, so just do the replacement
    view()->document()->replaceText(*m_completionRanges[model], model->data(toExecute.sibling(toExecute.row(), KTextEditor::CodeCompletionModel::Name)).toString());
  else
    model->executeCompletionItem(view()->document(), *m_completionRanges[model], toExecute.row());
  
  //Relock, because editEnd expects it to be locked
  view()->doc()->smartMutex()->lock();

  view()->doc()->editEnd();

  hide();

  view()->sendCompletionExecuted(start, model, toExecute);
  
  KTextEditor::Cursor newPos = view()->cursorPosition();
  if(newPos > oldPos) {
    m_automaticInvocationAt = newPos;
    m_automaticInvocationLine = view()->doc()->text(KTextEditor::Range(oldPos, newPos));
    kDebug() << "executed, starting automatic invocation with line" << m_automaticInvocationLine;
    m_lastInsertionByUser = false;
    m_automaticInvocationTimer->start();
  }
}

void KateCompletionWidget::resizeEvent( QResizeEvent * event )
{
  QFrame::resizeEvent(event);
}

void KateCompletionWidget::showEvent ( QShowEvent * event )
{
  m_isSuspended = false;

  QFrame::showEvent(event);

  if( !m_dontShowArgumentHints && m_argumentHintModel->rowCount(QModelIndex()) != 0 )
    m_argumentHintTree->show();
}

void KateCompletionWidget::hideEvent( QHideEvent * event )
{
  QFrame::hideEvent(event);
  m_argumentHintTree->hide();
  
  if(isCompletionActive()) {
    kDebug() << "completion active, aborting";
    abortCompletion();
  }
}

KateSmartRange * KateCompletionWidget::completionRange(KTextEditor::CodeCompletionModel* model) const
{
  if (!model) {
    if (m_sourceModels.isEmpty()) return 0;
    model = m_sourceModels.first();
  }
  return m_completionRanges[model];
}

QMap<KTextEditor::CodeCompletionModel*, KateSmartRange*> KateCompletionWidget::completionRanges( ) const
{
  return m_completionRanges;
}

void KateCompletionWidget::modelReset( )
{
  setUpdatesEnabled(false);
  m_entryList->setAnimated(false);
  m_argumentHintTree->setAnimated(false);
  ///We need to do this by hand, because QTreeView::expandAll is very inefficient.
  ///It creates a QPersistentModelIndex for every single item in the whole tree..
  for(int row = 0; row < m_argumentHintModel->rowCount(QModelIndex()); ++row) {
    QModelIndex index(m_argumentHintModel->index(row, 0, QModelIndex()));
    if(!m_argumentHintTree->isExpanded(index)) {
      m_argumentHintTree->expand(index);
    }
  }

  for(int row = 0; row < m_entryList->model()->rowCount(QModelIndex()); ++row) {
    QModelIndex index(m_entryList->model()->index(row, 0, QModelIndex()));
    if(!m_entryList->isExpanded(index)) {
      m_entryList->expand(index);
    }
  }
  setUpdatesEnabled(true);
}

KateCompletionTree* KateCompletionWidget::treeView() const {
  return m_entryList;
}

QModelIndex KateCompletionWidget::selectedIndex() const {
  if(!isCompletionActive())
    return QModelIndex();
  
  if( m_inCompletionList )
    return m_entryList->currentIndex();
  else
    return m_argumentHintTree->currentIndex();
}

bool KateCompletionWidget::navigateLeft() {
  m_hadCompletionNavigation = true;
  if(currentEmbeddedWidget())
    QMetaObject::invokeMethod(currentEmbeddedWidget(), "embeddedWidgetLeft");
  
  QModelIndex index = selectedIndex();

  if( index.isValid() ) {
    index.data(KTextEditor::CodeCompletionModel::AccessibilityPrevious);
    
    return true;
  }
  return false;
}

bool KateCompletionWidget::navigateRight() {
  m_hadCompletionNavigation = true;
  if(currentEmbeddedWidget()) ///@todo post 4.2: Make these slots public interface, or create an interface using virtual functions
    QMetaObject::invokeMethod(currentEmbeddedWidget(), "embeddedWidgetRight");
  
  QModelIndex index = selectedIndex();

  if( index.isValid() ) {
    index.data(KTextEditor::CodeCompletionModel::AccessibilityNext);
    return true;
  }

  return false;
}

bool KateCompletionWidget::hadNavigation() const {
  return m_hadCompletionNavigation;
}

void KateCompletionWidget::resetHadNavigation() {
  m_hadCompletionNavigation = false;
}


bool KateCompletionWidget::navigateBack() {
  m_hadCompletionNavigation = true;
  if(currentEmbeddedWidget())
    QMetaObject::invokeMethod(currentEmbeddedWidget(), "embeddedWidgetBack");
  return false;
}

bool KateCompletionWidget::toggleExpanded(bool forceExpand, bool forceUnExpand) {
  if ( (canExpandCurrentItem() || forceExpand ) && !forceUnExpand) {
    bool ret = canExpandCurrentItem();
    setCurrentItemExpanded(true);
    return ret;
  } else if (canCollapseCurrentItem() || forceUnExpand) {
    bool ret = canCollapseCurrentItem();
    setCurrentItemExpanded(false);
    return ret;
  }
  return false;
}

bool KateCompletionWidget::canExpandCurrentItem() const {
  if( m_inCompletionList ) {
    if( !m_entryList->currentIndex().isValid() ) return false;
    return model()->isExpandable( m_entryList->currentIndex() ) && !model()->isExpanded( m_entryList->currentIndex() );
  } else {
    if( !m_argumentHintTree->currentIndex().isValid() ) return false;
    return argumentHintModel()->isExpandable( m_argumentHintTree->currentIndex() ) && !argumentHintModel()->isExpanded( m_argumentHintTree->currentIndex() );
  }
}

bool KateCompletionWidget::canCollapseCurrentItem() const {
  if( m_inCompletionList ) {
    if( !m_entryList->currentIndex().isValid() ) return false;
    return model()->isExpandable( m_entryList->currentIndex() ) && model()->isExpanded( m_entryList->currentIndex() );
  }else{
    if( !m_argumentHintTree->currentIndex().isValid() ) return false;
    return m_argumentHintModel->isExpandable( m_argumentHintTree->currentIndex() ) && m_argumentHintModel->isExpanded( m_argumentHintTree->currentIndex() );
  }
}

void KateCompletionWidget::setCurrentItemExpanded( bool expanded ) {
  if( m_inCompletionList ) {
    if( !m_entryList->currentIndex().isValid() ) return;
    model()->setExpanded(m_entryList->currentIndex(), expanded);
    updateHeight();
  }else{
    if( !m_argumentHintTree->currentIndex().isValid() ) return;
    m_argumentHintModel->setExpanded(m_argumentHintTree->currentIndex(), expanded);
  }
}

void KateCompletionWidget::startCharacterDeleted( KTextEditor::SmartCursor*, bool deletedBefore )
{
  if (deletedBefore)
    // Cannot abort completion from this function, or the cursor will be deleted before returning
    QTimer::singleShot(0, this, SLOT(abortCompletion()));
}

bool KateCompletionWidget::eventFilter( QObject * watched, QEvent * event )
{
  bool ret = QFrame::eventFilter(watched, event);

  if (watched != this)
    if (event->type() == QEvent::Move)
      updatePosition();

  return ret;
}

bool KateCompletionWidget::navigateDown() {
  m_hadCompletionNavigation = true;
  if(currentEmbeddedWidget()) {
    QMetaObject::invokeMethod(currentEmbeddedWidget(), "embeddedWidgetDown");
  }
  return false;
}

bool KateCompletionWidget::navigateUp() {
  m_hadCompletionNavigation = true;
  if(currentEmbeddedWidget())
    QMetaObject::invokeMethod(currentEmbeddedWidget(), "embeddedWidgetUp");
  return false;
}

QWidget* KateCompletionWidget::currentEmbeddedWidget() {
  QModelIndex index = selectedIndex();
  if(!index.isValid())
    return 0;
  if( qobject_cast<const ExpandingWidgetModel*>(index.model()) ) {
    const ExpandingWidgetModel* model = static_cast<const ExpandingWidgetModel*>(index.model());
    if( model->isExpanded(index) )
      return model->expandingWidget(index);
  }
  return 0;
}

void KateCompletionWidget::cursorDown()
{
  if( m_inCompletionList )
    m_entryList->nextCompletion();
  else {
    if( !m_argumentHintTree->nextCompletion() )
      switchList();
  }
}

void KateCompletionWidget::cursorUp()
{
  if( m_inCompletionList ) {
    if( !m_entryList->previousCompletion() )
      switchList();
  }else{
    m_argumentHintTree->previousCompletion();
  }
}

void KateCompletionWidget::pageDown( )
{
  if( m_inCompletionList )
    m_entryList->pageDown();
  else {
    if( !m_argumentHintTree->pageDown() )
      switchList();
  }
}

void KateCompletionWidget::pageUp( )
{
  if( m_inCompletionList ) {
    if( !m_entryList->pageUp() )
      switchList();
  }else{
    m_argumentHintTree->pageUp();
  }
}

void KateCompletionWidget::top( )
{
  if( m_inCompletionList )
    m_entryList->top();
  else
    m_argumentHintTree->top();
}

void KateCompletionWidget::bottom( )
{
  if( m_inCompletionList )
    m_entryList->bottom();
  else
    m_argumentHintTree->bottom();
}

void KateCompletionWidget::switchList() {
  if( m_inCompletionList ) {
      m_entryList->setCurrentIndex(QModelIndex());
      if( m_argumentHintModel->rowCount(QModelIndex()) != 0 )
        m_argumentHintTree->setCurrentIndex(m_argumentHintModel->index(m_argumentHintModel->rowCount(QModelIndex())-1, 0));
  } else {
      m_argumentHintTree->setCurrentIndex(QModelIndex());
      if( m_presentationModel->rowCount(QModelIndex()) != 0 )
        m_entryList->setCurrentIndex(m_presentationModel->index(0, 0));
  }
  m_inCompletionList = !m_inCompletionList;
}

void KateCompletionWidget::showConfig( )
{
  abortCompletion();

  m_configWidget->exec();
}

void KateCompletionWidget::registerCompletionModel(KTextEditor::CodeCompletionModel* model)
{
  m_sourceModels.append(model);

  if (isCompletionActive()) {
    m_presentationModel->addCompletionModel(model);
  }
}

void KateCompletionWidget::unregisterCompletionModel(KTextEditor::CodeCompletionModel* model)
{
  m_sourceModels.removeAll(model);
}

int KateCompletionWidget::automaticInvocationDelay() const {
  return m_automaticInvocationDelay;
}

void KateCompletionWidget::setAutomaticInvocationDelay(int delay) {
  m_automaticInvocationDelay = delay;
}


void KateCompletionWidget::editDone(KateEditInfo * edit)
{
  if(!edit->newText().join("\n").trimmed().isEmpty())
  m_lastInsertionByUser = edit->editSource() == Kate::UserInputEdit;
  
  if (!view()->config()->automaticCompletionInvocation()
       || (edit->editSource() != Kate::UserInputEdit)
       || edit->isRemoval()
       || (edit->editSource() != Kate::UserInputEdit && edit->editSource() != Kate::CodeCompletionEdit)
       || edit->newText().isEmpty() )
  {
    m_automaticInvocationLine.clear();
    m_automaticInvocationTimer->stop();
    return;
  }

  if(m_automaticInvocationAt != edit->newRange().start()) {
    m_automaticInvocationLine.clear();
    m_lastInsertionByUser = edit->editSource() == Kate::UserInputEdit;
  }

  m_automaticInvocationLine += edit->newText().last();
  m_automaticInvocationAt = edit->newRange().end();

  if (m_automaticInvocationLine.isEmpty()) {
    m_automaticInvocationTimer->stop();
    return;
  }

  m_automaticInvocationTimer->start(m_automaticInvocationDelay);
}

void KateCompletionWidget::automaticInvocation()
{
  if(m_automaticInvocationAt != view()->cursorPosition())
    return;
  bool start = false;
  
  foreach (KTextEditor::CodeCompletionModel *model, m_sourceModels) {
      if(m_completionRanges.contains(model))
        continue;
      start = modelController(model)->shouldStartCompletion(view(), m_automaticInvocationLine, m_lastInsertionByUser, view()->cursorPosition());
      if (start) break;
  }
  if (start) {
    // Start automatic code completion
    startCompletion(KTextEditor::CodeCompletionModel::AutomaticInvocation);
  }
}

void KateCompletionWidget::userInvokedCompletion()
{
  startCompletion(KTextEditor::CodeCompletionModel::UserInvocation);
}

#include "katecompletionwidget.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
