/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
                                      
#include "kateconfigplugindialogpage.h"
#include "kateconfigplugindialogpage.moc"

#include "katepluginmanager.h"
#include "kateconfigdialog.h"
#include <klistbox.h>
#include "kateapp.h"
#include <qstringlist.h>
#include <qhbox.h>
#include <qlabel.h>
#include <klocale.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <kiconloader.h>
#include <qwhatsthis.h>

class KatePluginListItem : public QCheckListItem
{
  public:
    KatePluginListItem(bool checked, KatePluginInfo *info, QListView *parent);
    KatePluginInfo *info() const { return mInfo; }
  
  protected:	
    void stateChange(bool);
    
  private:
    KatePluginInfo *mInfo;
    bool silentStateChange;
};
         
KatePluginListItem::KatePluginListItem(bool checked, KatePluginInfo *info, QListView *parent)
  : QCheckListItem(parent, info->service->name(), CheckBox)
  , mInfo(info)
  , silentStateChange(false)
{
  silentStateChange = true;
  setOn(checked);
  silentStateChange = false;
}

void KatePluginListItem::stateChange(bool b)
{
  if(!silentStateChange)
    static_cast<KatePluginListView *>(listView())->stateChanged(this, b);
}

KatePluginListView::KatePluginListView(QWidget *parent, const char *name)
  : KListView(parent, name)
{
}

void KatePluginListView::stateChanged(KatePluginListItem *item, bool b)
{
  emit stateChange(item, b);
}

KateConfigPluginPage::KateConfigPluginPage(QWidget *parent, KateConfigDialog *dialog):QVBox(parent)
{
  myPluginMan=((KateApp*)kapp)->katePluginManager();
  myDialog=dialog;
  
  KatePluginListView* listView = new KatePluginListView(this);
  listView->addColumn(i18n("Name"));
  listView->addColumn(i18n("Comment"));
  QWhatsThis::add(listView,i18n("Here you can see all available Kate plugins. Those with a check mark are loaded, and will be loaded again the next time Kate is started."));

  connect(listView, SIGNAL(stateChange(KatePluginListItem *, bool)), this, SLOT(stateChange(KatePluginListItem *, bool)));
      
  for (uint i=0; i<myPluginMan->pluginList().count(); i++)
  {
    KatePluginListItem *item = new KatePluginListItem(myPluginMan->pluginList().at(i)->load, myPluginMan->pluginList().at(i), listView);
    item->setText(0, myPluginMan->pluginList().at(i)->service->name());
    item->setText(1, myPluginMan->pluginList().at(i)->service->comment());
  }
}

 void KateConfigPluginPage::stateChange(KatePluginListItem *item, bool b)
{   
  if(b)
    loadPlugin(item);
  else
    unloadPlugin(item);
  
  emit changed();
}
                      
void KateConfigPluginPage::loadPlugin (KatePluginListItem *item)
{       
  myPluginMan->loadPlugin (item->info());
  myPluginMan->enablePluginGUI (item->info());
  myDialog->addPluginPage (item->info()->plugin);
   
  item->setOn(true);
}

void KateConfigPluginPage::unloadPlugin (KatePluginListItem *item)
{                                  
  myDialog->removePluginPage (item->info()->plugin);
  myPluginMan->unloadPlugin (item->info());
    
  item->setOn(false);
}
