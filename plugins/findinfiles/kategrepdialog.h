/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _GREPDIALOG_H_
#define _GREPDIALOG_H_

#include <kate/interfaces/mainwindow.h>

#include "kategrepthread.h"
#include "ui_findwidget.h"

#include <KDialog>
#include <KConfig>
#include <QStringList>
#include <QLabel>
#include <QEvent>

class QLineEdit;
class KComboBox;
class QCheckBox;
class QTreeWidget;
class QTreeWidgetItem;
class KPushButton;
class QLabel;
class KProcess;
class KConfig;
class KUrlRequester;
class QEvent;

class KateGrepDialog : public QWidget, private Ui::FindWidget
{
    Q_OBJECT

public:
    KateGrepDialog(QWidget *parent, Kate::MainWindow *mw);
    ~KateGrepDialog();

protected:
    bool eventFilter( QObject *, QEvent * );
    void showEvent(QShowEvent* event);

private Q_SLOTS:
    void itemSelected(QTreeWidgetItem *item, int column);
    void slotSearch();
    void slotClear();
    void patternTextChanged( const QString &);
    void searchFinished ();
    void searchMatchFound(const QString &filename, int line, int column, const QString &basename, const QString &lineContent);
    void syncDir();

private:
    void killThread ();

    Kate::MainWindow *m_mw;
    QString buf;
    QString errbuf;
    KSharedConfigPtr config;
    QStringList lastSearchItems;
    QStringList lastSearchPaths;
    QStringList lastSearchFiles;
    QString m_lastUpdatedDir;
    QString m_workingDir;

    KateGrepThread *m_grepThread;
};


#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
