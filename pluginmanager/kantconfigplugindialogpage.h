#ifndef _kant_config_Plugin_Dialog_Page_h_
#define _kant_config_Plugin_Dialog_Page_h_

#include "../kantmain.h"

#include <qvbox.h>

class QListBoxItem;

class KantConfigPluginPage: public QVBox
{
  Q_OBJECT

  public:
    KantConfigPluginPage(QWidget *parent);
    ~KantConfigPluginPage(){;};

  private:
    KantPluginManager *myPluginMan;

    KListBox *availableBox;
    KListBox *loadedBox;
    class QLabel *label;

  private slots:
    void slotUpdate ();
    void slotActivatePluginItem (QListBoxItem *item);
};

#endif
