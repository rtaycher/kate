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
  if  (myPluginMan->availablePlugins.count() > 0)
    availableBox->insertStringList (myPluginMan->availablePlugins);

  loadedBox->clear();
  if  (myPluginMan->loadedPlugins.count() > 0)
    loadedBox->insertStringList (myPluginMan->loadedPlugins);
}
