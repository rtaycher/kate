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

#ifndef __KATE_CONFIGPLUGINDIALOGPAGE_H__
#define __KATE_CONFIGPLUGINDIALOGPAGE_H__

#include "katemain.h"
#include "katepluginmanager.h"

#include <qvbox.h>  

#define private protected
#include <qlistview.h>
#undef private 

#include <klistview.h>

class QListBoxItem;

class PluginListItem : public QCheckListItem
{
public:
	PluginListItem(const bool _exclusive, bool _checked, PluginInfo *_info, QListView *_parent);
	PluginInfo *info() const { return mInfo; }

	// This will toggle the state without "emitting" the stateChange
	void setChecked(bool);

protected:	
	virtual void stateChange(bool);
	virtual void paintCell(QPainter *, const QColorGroup &, int, int, int);
   
private:
	PluginInfo *mInfo;
	bool silentStateChange;
	bool exclusive;
};

class PluginListView : public KListView
{
Q_OBJECT

friend class PluginListItem;

public:
	PluginListView(QWidget *_parent = 0, const char *_name = 0);
	PluginListView(unsigned _min, QWidget *_parent = 0, const char *_name = 0);
	PluginListView(unsigned _min, unsigned _max, QWidget *_parent = 0, const char *_name = 0);

	virtual void clear();

signals:
	void stateChange(PluginListItem *, bool);

private:
	void stateChanged(PluginListItem *, bool);
	
	bool hasMaximum;
	unsigned max, min;
	unsigned count;
};

class KateConfigPluginPage: public QVBox
{
  Q_OBJECT

  public:
    KateConfigPluginPage(QWidget *parent, class KateConfigDialog *dialog);
    ~KateConfigPluginPage(){;};

  private:
    KatePluginManager *myPluginMan;
    class KateConfigDialog *myDialog;

  signals:
    void changed();

  private slots:
    void stateChange(PluginListItem *, bool);
    
    void loadPlugin (PluginListItem *);
    void unloadPlugin (PluginListItem *);
};

#endif
