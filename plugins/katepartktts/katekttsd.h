/***************************************************************************
  Copyright:
  (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KATEKTTSD_H_
#define _KATEKTTSD_H_

// $(kde_includes)/kate
#include <mainwindow.h>
#include <plugin.h>

#include <klibloader.h>
#include <klocale.h>

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

class KatePluginKTTSD : public Kate::Plugin, Kate::PluginViewInterface
{
  Q_OBJECT

  public:
    KatePluginKTTSD( QObject* parent = 0, const char* name = 0 );
    virtual ~KatePluginKTTSD();

    void addView (Kate::MainWindow *win);
    void removeView (Kate::MainWindow *win);
    
  public slots:
    void slotReadOut();  
    
  private:
    QPtrList<class PluginView> m_views; 
};

#endif
