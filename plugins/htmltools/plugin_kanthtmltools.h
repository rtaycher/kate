 /***************************************************************************
                          plugin_kanthtmltools.h  -  description
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

#include <kantplugin.h>
#include <kantappIface.h>

#include <klibloader.h>

class KantPluginFactory : public KLibFactory
{
  Q_OBJECT

  public:
    KantPluginFactory();
    virtual ~KantPluginFactory();

    virtual QObject* createObject( QObject* parent = 0, const char* pname = 0, const char* name = "QObject", const QStringList &args = QStringList() );

  private:
    static KInstance* s_instance;
};

class PluginKantHtmlTools : public KantPlugin
{
  Q_OBJECT
public:
  PluginKantHtmlTools( QObject* parent = 0, const char* name = 0 );
  virtual ~PluginKantHtmlTools();

  KantPluginView *createView ();

private:
  KantAppIface *myParent;

  QString KantPrompt (QString strTitle, QString strPrompt,
			     QWidget * that);
  void slipInHTMLtag (KantView & view, QString text);

public slots:
  void slotEditHTMLtag();
};

#endif // _PLUGIN_KANT_HTMLTOOLS_H
