 /***************************************************************************
                          plugin_kanttextfilter.h  -  description
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

#ifndef _PLUGIN_KANT_TEXTFILTER_H
#define _PLUGIN_KANT_TEXTFILTER_H

#include <qstring.h>

#include <kparts/part.h>
#include <klibloader.h>
#include <kantpluginIface.h>

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

class PluginKantTextFilter : public KParts::Part
{
  Q_OBJECT

  public:
    PluginKantTextFilter( QObject* parent = 0, const char* name = 0 );
    virtual ~PluginKantTextFilter();

  private:
    KantPluginIface *myParent;
    QString  m_strFilterOutput;
    KShellProcess * m_pFilterShellProcess;

  public slots:
    void slotEditFilter ();
    void slotFilterReceivedStdout (KProcess * pProcess, char * got, int len);
    void slotFilterReceivedStderr (KProcess * pProcess, char * got, int len);
    void slotFilterProcessExited (KProcess * pProcess);
    void slotFilterCloseStdin (KProcess *);
};

#endif // _PLUGIN_KANT_TEXTFILTER_H
