 /***************************************************************************
                          plugin_katehtmltools.h  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _PLUGIN_KANT_HTMLTOOLS_H
#define _PLUGIN_KANT_HTMLTOOLS_H

#include <qstring.h>

#include "../../interfaces/plugin.h"
#include "../../interfaces/application.h"
#include "../../interfaces/view.h"
#include "../../interfaces/document.h"
#include "../../interfaces/docmanager.h"
#include "../../interfaces/viewmanager.h"

#include <klibloader.h>

class KatePluginFactory : public KLibFactory
{
  Q_OBJECT

  public:
    KatePluginFactory();
    virtual ~KatePluginFactory();

    virtual QObject* createObject( QObject* parent = 0, const char* pname = 0, const char* name = "QObject", const QStringList &args = QStringList() );

  private:
    static KInstance* s_instance;
};

class PluginKateHtmlTools : public Kate::Plugin
{
  Q_OBJECT
public:
  PluginKateHtmlTools( QObject* parent = 0, const char* name = 0 );
  virtual ~PluginKateHtmlTools();

  Kate::PluginView *createView ();

private:

  QString KatePrompt (QString strTitle, QString strPrompt,
			     QWidget * that);
  void slipInHTMLtag (Kate::View & view, QString text);

public slots:
  void slotEditHTMLtag();
};

#endif // _PLUGIN_KANT_HTMLTOOLS_H
