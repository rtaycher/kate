/***************************************************************************
                          kateconfigplugindialogpage.cpp  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
                                                     
// Copyright (c) 2000-2001 Charles Samuels <charles@kde.org>
// Copyright (c) 2000-2001 Neil Stevens <multivac@fcmail.com>
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIAB\ILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
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
  listView->addColumn(i18n("Description"));
  listView->addColumn(i18n("Author"));
  listView->addColumn(i18n("License"));
  connect(listView, SIGNAL(stateChange(PluginListItem *, bool)), this, SLOT(stateChange(PluginListItem *, bool)));
      
  for (uint i=0; i<myPluginMan->pluginList().count(); i++)
  {
    PluginListItem *item = new PluginListItem(false, myPluginMan->pluginList().at(i)->load, myPluginMan->pluginList().at(i), listView);
    item->setText(0, myPluginMan->pluginList().at(i)->service->name());
    item->setText(1, myPluginMan->pluginList().at(i)->service->comment());
    item->setText(2, "");
    item->setText(3, "");
  }
}

 void KateConfigPluginPage::stateChange(PluginListItem *item, bool b)
{   
	if(b)
		loadPlugin(item);
	else
		unloadPlugin(item);
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
