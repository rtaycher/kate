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

// $Id$
                                                     
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
         
PluginListItem::PluginListItem(const bool _exclusive, bool _checked, PluginInfo *_info, QListView *_parent)
	: QCheckListItem(_parent, _info->service->name(), CheckBox)
	, mInfo(_info)
	, silentStateChange(false)
	, exclusive(_exclusive)
{
	setChecked(_checked);
	if(_checked) static_cast<PluginListView *>(listView())->count++;
}


void PluginListItem::setChecked(bool b)
{
	silentStateChange = true;
	setOn(b);
	silentStateChange = false;
}

void PluginListItem::stateChange(bool b)
{
	if(!silentStateChange)
		static_cast<PluginListView *>(listView())->stateChanged(this, b);
}

void PluginListItem::paintCell(QPainter *p, const QColorGroup &cg, int a, int b, int c)
{
	if(exclusive) myType = RadioButton;
	QCheckListItem::paintCell(p, cg, a, b, c);
	if(exclusive) myType = CheckBox;
}

PluginListView::PluginListView(unsigned _min, unsigned _max, QWidget *_parent, const char *_name)
	: KListView(_parent, _name)
	, hasMaximum(true)
	, max(_max)
	, min(_min <= _max ? _min : _max)
	, count(0)
{
}

PluginListView::PluginListView(unsigned _min, QWidget *_parent, const char *_name)
	: KListView(_parent, _name)
	, hasMaximum(false)
	, min(_min)
	, count(0)
{
}

PluginListView::PluginListView(QWidget *_parent, const char *_name)
	: KListView(_parent, _name)
	, hasMaximum(false)
	, min(0)
	, count(0)
{
}

void PluginListView::clear()
{
	count = 0;
	KListView::clear();
}

void PluginListView::stateChanged(PluginListItem *item, bool b)
{
	if(b)
	{
		count++;
		emit stateChange(item, b);
		
		if(hasMaximum && count > max)
		{
			// Find a different one and turn it off

			QListViewItem *cur = firstChild();
			PluginListItem *curItem = dynamic_cast<PluginListItem *>(cur);

			while(cur == item || !curItem || !curItem->isOn())
			{
				cur = cur->nextSibling();
				curItem = dynamic_cast<PluginListItem *>(cur);
			}

			curItem->setOn(false);
		}
	}
	else
	{
		if(count == min)
		{
			item->setChecked(true);
		}
		else
		{
			count--;
			emit stateChange(item, b);
		}
	}
}

KateConfigPluginPage::KateConfigPluginPage(QWidget *parent, KateConfigDialog *dialog):QVBox(parent)
{
  myPluginMan=((KateApp*)kapp)->katePluginManager();
  myDialog=dialog;
  
  PluginListView* listView = new PluginListView(0, this);
  listView->addColumn(i18n("Name"));
  listView->addColumn(i18n("Comment"));
  QWhatsThis::add(listView,i18n("Here you can see all available Kate plugins. Those with a check mark are loaded, and will be loaded again the next time Kate is started."));

  connect(listView, SIGNAL(stateChange(PluginListItem *, bool)), this, SLOT(stateChange(PluginListItem *, bool)));
      
  for (uint i=0; i<myPluginMan->pluginList().count(); i++)
  {
    PluginListItem *item = new PluginListItem(false, myPluginMan->pluginList().at(i)->load, myPluginMan->pluginList().at(i), listView);
    item->setText(0, myPluginMan->pluginList().at(i)->service->name());
    item->setText(1, myPluginMan->pluginList().at(i)->service->comment());
  }
}

 void KateConfigPluginPage::stateChange(PluginListItem *item, bool b)
{   
	if(b)
		loadPlugin(item);
	else
		unloadPlugin(item);
	emit changed();
}
                      
void KateConfigPluginPage::loadPlugin (PluginListItem *item)
{       
  myPluginMan->loadPlugin (item->info());
  myPluginMan->enablePluginGUI (item->info());
  myDialog->addPluginPage (item->info()->plugin);
   
  item->setOn(true);
}

void KateConfigPluginPage::unloadPlugin (PluginListItem *item)
{                                  
  myDialog->removePluginPage (item->info()->plugin);
  myPluginMan->unloadPlugin (item->info());
    
  item->setOn(false);
}
