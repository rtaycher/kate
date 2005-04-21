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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _GREPDIALOG_H_
#define _GREPDIALOG_H_

#include <kdialog.h>
#include <qstringlist.h>

class QLineEdit;
class QComboBox;
class QCheckBox;
class QListBox;
class KPushButton;
class QLabel;
class KProcess;
class KConfig;
class KURLRequester;
class QEvent;

class GrepTool : public QWidget
{
    Q_OBJECT

public:
    GrepTool(class KateMainWindow *parent, QWidget *p, const char *name=0);
    ~GrepTool();

    // only updates if the dir you give to it differs from the last one given to it !
    void updateDirName(const QString &);

    void setDirName(const QString &);


signals:
    void itemSelected(const QString &abs_filename, int line);

public slots:
    void slotSearchFor(const QString &pattern);

protected:
    bool eventFilter( QObject *, QEvent * );

private slots:
    void templateActivated(int index);
    void childExited();
    void receivedOutput(KProcess *proc, char *buffer, int buflen);
    void receivedErrOutput(KProcess *proc, char *buffer, int buflen);
    void itemSelected(const QString&);
    void slotSearch();
    void slotCancel();
    void slotClear();
    void patternTextChanged( const QString &);
private:
    void processOutput();
    void finish();

    QLineEdit *leTemplate;
    QComboBox *cmbFiles, *cmbPattern;
    KURLRequester *cmbDir;
    QCheckBox *cbRecursive;
    QCheckBox *cbCasesensitive, *cbRegex;
    QListBox *lbResult;
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
