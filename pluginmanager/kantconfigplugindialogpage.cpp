#include "kantconfigplugindialogpage.h"
#include "kantconfigplugindialogpage.moc"

#include "kantpluginmanager.h"
#include <qlistbox.h>
#include "../app/kantapp.h"

KantConfigPluginPage::KantConfigPluginPage(QWidget *parent):QHBox(parent)
{
  AvailableBox=new QListBox(this);
  LoadBox=new QListBox(this);

  KantPluginManager *man=((KantApp*)kapp)->getPluginManager();
}

