/***************************************************************************
                          katefileselector.h  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Matt Newell
    email                : newellm@proaxis.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __KATE_FILESELECTOR_H__
#define __KATE_FILESELECTOR_H__

#include "katemain.h"

#include <qwidget.h>
#include <kfile.h>
#include <kurl.h>

class KateMainWindow;
class KateViewManager;

class KateFileSelector : public QWidget
{
  Q_OBJECT

  public:
    KateFileSelector( KateMainWindow *mainWindow=0, KateViewManager *viewManager=0,
                      QWidget * parent = 0, const char * name = 0 );
    ~KateFileSelector();

    void readConfig(KConfig *, const QString &);
    void writeConfig(KConfig *, const QString &);
    void setView(KFile::FileView);
    KDirOperator * dirOperator(){return dir;}

  public slots:
    void slotFilterChange(const QString&);
    void setDir(KURL);

  private slots:
    void cmbPathActivated( const KURL& u );
    void cmbPathReturnPressed( const QString& u );
    void dirUrlEntered( const KURL& u );
    void dirFinishedLoading();
    void setCurrentDocDir();
    void kateViewChanged();

  protected:
    void focusInEvent(QFocusEvent*);

  private:
    KURLComboBox *cmbPath;
    KHistoryCombo * filter;
    QLabel* filterIcon;
    KDirOperator * dir;
    class QToolButton *home, *up, *back, *forward, *cfdir;
    
    KateMainWindow *mainwin;
    KateViewManager *viewmanager;
};

#endif //KANTFILESELECTOR_H
