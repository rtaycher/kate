#include "kantconfigplugindialogpage.h"
#include "kantconfigplugindialogpage.moc"

#include "kantpluginmanager.h"
#include <klistbox.h>
#include "../app/kantapp.h"
#include <qstringlist.h>
#include <qhbox.h>
#include <qlabel.h>
#include <klocale.h>

KantConfigPluginPage::KantConfigPluginPage(QWidget *parent):QVBox(parent)
{
  myPluginMan=((KantApp*)kapp)->getPluginManager();

  QHBox *hbox = new QHBox (this);

  availableBox=new KListBox(hbox);
  loadedBox=new KListBox(hbox);

  label = new QLabel (this);
  label->setMinimumHeight (50);
  label->setText (i18n("Select a plugin to get a short info here !"));

  connect(availableBox,SIGNAL(highlighted(QListBoxItem *)),this,SLOT(slotActivatePluginItem (QListBoxItem *)));
  connect(availableBox,SIGNAL(selected(QListBoxItem *)), this,SLOT(slotActivatePluginItem (QListBoxItem *)));

  connect(loadedBox,SIGNAL(highlighted(QListBoxItem *)),this,SLOT(slotActivatePluginItem (QListBoxItem *)));
  connect(loadedBox,SIGNAL(selected(QListBoxItem *)), this,SLOT(slotActivatePluginItem (QListBoxItem *)));

  slotUpdate();
}

void KantConfigPluginPage::slotUpdate ()
{
  availableBox->clear();
  loadedBox->clear();

  for (int i=0; i<myPluginMan->myPluginList.count(); i++)
  {
    if  (!myPluginMan->myPluginList.at(i)->load)
      availableBox->insertItem (myPluginMan->myPluginList.at(i)->name);
    else
      loadedBox->insertItem (myPluginMan->myPluginList.at(i)->name);
  }
}

void KantConfigPluginPage::slotActivatePluginItem (QListBoxItem *item)
{
  for (int i=0; i<myPluginMan->myPluginList.count(); i++)
  {
    if  (myPluginMan->myPluginList.at(i)->name == item->text())
      label->setText (i18n("Name: ") + myPluginMan->myPluginList.at(i)->name + i18n ("\nAuthor: ") + myPluginMan->myPluginList.at(i)->author + i18n ("\nDescription: ") + myPluginMan->myPluginList.at(i)->description);
  }
}
