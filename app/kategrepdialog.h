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

#include <kdialog.h>
#include <QStringList>
//Added by qt3to4:
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

class GrepTool : public QWidget
{
    Q_OBJECT

public:
    GrepTool(QWidget *parent);
    ~GrepTool();

    // only updates if the dir you give to it differs from the last one given to it !
    void updateDirName(const QString &);

    void setDirName(const QString &);


Q_SIGNALS:
    void itemSelected(const QString &abs_filename, int line);

public Q_SLOTS:
    void slotSearchFor(const QString &pattern);

protected:
    bool eventFilter( QObject *, QEvent * );

private Q_SLOTS:
    void templateActivated(int index);
    void childExited();
    void receivedOutput(KProcess *proc, char *buffer, int buflen);
    void receivedErrOutput(KProcess *proc, char *buffer, int buflen);
    void itemSelected(QTreeWidgetItem *item, int column);
    void slotSearch();
    void slotCancel();
    void slotClear();
    void patternTextChanged( const QString &);

private:
    void processOutput();
    void finish();

    QLineEdit *leTemplate;
    KComboBox *cmbFiles, *cmbPattern;
    KUrlRequester *cmbDir;
    QCheckBox *cbRecursive;
    QCheckBox *cbCasesensitive, *cbRegex;
    QTreeWidget *lbResult;
    KPushButton *btnSearch, *btnClear;
    KProcess *childproc;
    QString buf;
    QString errbuf;
    KConfig* config;
    QStringList lastSearchItems;
    QStringList lastSearchPaths;
    QStringList lastSearchFiles;
    QString m_lastUpdatedDir;
    QString m_workingDir;
};


#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
