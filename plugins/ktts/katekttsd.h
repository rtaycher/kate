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
#include <plugin.h>

class KatePluginKTTSD : public Kate::Plugin, Kate::PluginViewInterface
{
  Q_OBJECT

  public:
    KatePluginKTTSD( QObject* parent = 0, const char* name = 0, const QStringList& = QStringList() );
    virtual ~KatePluginKTTSD();

    void addView (Kate::MainWindow *win);
    void removeView (Kate::MainWindow *win);
    
  public slots:
    void slotReadOut();  
    
  private:
    QPtrList<class PluginView> m_views; 
};

#endif
