/***************************************************************************
                          kantfileselector.h  -  description
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

#ifndef KANTFILESELECTOR_H
#define KANTFILESELECTOR_H

#include "../kantmain.h"
#include "../mainwindow/kantmainwindow.h"
#include <qwidget.h>

#include <kfile.h>

class QGridLayout;
class KURLComboBox;
class KHistoryCombo;
class KDirOperator;
class KConfig;
class KURL;
class QString;
class QLabel;

class KantFileSelector : public QWidget
{
  Q_OBJECT

  public:
    KantFileSelector(QWidget * parent = 0, const char * name = 0 );
    ~KantFileSelector();

    void readConfig(KConfig *, const QString &);
    void saveConfig(KConfig *, const QString &);
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

  protected:
    void focusInEvent(QFocusEvent*);
  private:
    KURLComboBox *cmbPath;
    KHistoryCombo * filter;
    QLabel* filterIcon;
    KDirOperator * dir;
    QPushButton *home, *up, *back, *forward, *cfdir;
};

#endif //KANTFILESELECTOR_H
