/***************************************************************************
                          kateconfigplugindialogpage.h  -  description
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

  private slots:
    void stateChange(PluginListItem *, bool);
    
    void loadPlugin (PluginListItem *);
    void unloadPlugin (PluginListItem *);
};

#endif
