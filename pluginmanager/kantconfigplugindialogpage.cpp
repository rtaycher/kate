#include "kantconfigplugindialogpage.h"
#include "kantconfigplugindialogpage.moc"

#include "kantpluginmanager.h"
#include <klistbox.h>
#include "../app/kantapp.h"
#include <qstringlist.h>

KantConfigPluginPage::KantConfigPluginPage(QWidget *parent):QHBox(parent)
{
  KantPluginManager *myPluginMan=((KantApp*)kapp)->getPluginManager();

  availableBox=new KListBox(this);
  loadedBox=new KListBox(this);

   availableBox->show();
 // loadedBox->clear();
  loadedBox->show();

  slotUpdate();
}

void KantConfigPluginPage::slotUpdate ()
{
  //availableBox->clear();

  availableBox->insertStringList (myPluginMan->availablePlugins);
 // loadedBox->clear();
  loadedBox->insertStringList (myPluginMan->loadedPlugins);
}
