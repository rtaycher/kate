#ifndef _kant_config_Plugin_Dialog_Page_h_
#define _kant_config_Plugin_Dialog_Page_h_

#include "../kantmain.h"

#include <qhbox.h>

class KantConfigPluginPage: public QHBox
{
  Q_OBJECT

  public:
    KantConfigPluginPage(QWidget *parent);
    ~KantConfigPluginPage(){;};

  private:
    QListBox *AvailableBox;
    QListBox *LoadBox;
};

#endif
