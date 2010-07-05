#include "katerecoverbar.h"
#include "ui_recoverwidget.h"
#include "kateswapfile.h"
#include "kateview.h"

//BEGIN KateRecoverBar
KateRecoverBar::KateRecoverBar(KateView *view, QWidget *parent)
  : KateViewBarWidget( true, parent )
  , m_view( view )
{
  Ui::RecoverWidget* ui = new Ui::RecoverWidget();
  ui->setupUi( centralWidget() );

  // use queued connections because this (all) KateRecoverBar widgets are deleted
  connect(ui->btnRecover, SIGNAL(clicked()), m_view->doc()->swapFile(), SLOT(recover()), Qt::QueuedConnection);
  connect(ui->btnDiscard, SIGNAL(clicked()), m_view->doc()->swapFile(), SLOT(discard()), Qt::QueuedConnection);
  connect(ui->btnDiff, SIGNAL(clicked()), this, SLOT(viewDiff()));
}

void KateRecoverBar::viewDiff()
{
}
//END KateRecoverBar

// kate: space-indent on; indent-width 2; replace-tabs on;
