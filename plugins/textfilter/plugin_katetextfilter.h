 /***************************************************************************
                          plugin_katetextfilter.h  -  description
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

#include <katepluginIface.h>

#include <klibloader.h>
#include <qstring.h>
#include <kprocess.h>

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

class PluginKateTextFilter : public KatePluginIface
{
  Q_OBJECT

  public:
    PluginKateTextFilter( QObject* parent = 0, const char* name = 0 );
    virtual ~PluginKateTextFilter();

    KatePluginViewIface *createView ();

  private:
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
