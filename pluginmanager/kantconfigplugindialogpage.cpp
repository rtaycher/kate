#include "kantconfigplugindialogpage.h"
#include "kantconfigplugindialogpage.moc"

#include "kantpluginmanager.h"
#include <klistbox.h>
#include "../app/kantapp.h"
#include <qstringlist.h>

KantConfigPluginPage::KantConfigPluginPage(QWidget *parent):QHBox(parent)
{
  myPluginMan=((KantApp*)kapp)->getPluginManager();

  availableBox=new KListBox(this);
  loadedBox=new KListBox(this);

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
