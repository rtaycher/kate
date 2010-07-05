#ifndef _KATE_RECOVER_H__
#define _KATE_RECOVER_H__

#include "kateviewhelpers.h"

class KateView;

class KateRecoverBar : public KateViewBarWidget
{
  Q_OBJECT

  public:
    explicit KateRecoverBar(KateView *view, QWidget *parent = 0);

  protected Q_SLOTS:
    void recover();
    void viewDiff();

  private:
    KateView *const m_view;
};

#endif //_KATE_RECOVER_H__

// kate: space-indent on; indent-width 2; replace-tabs on;
