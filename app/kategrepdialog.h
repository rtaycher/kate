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
class QPushButton;
class QLabel;
class KProcess;
class KConfig;
class KURLRequester;

class GrepDialog : public QWidget
{
    Q_OBJECT

public:
    GrepDialog(const QString &dirname, class KateMainWindow *parent=0, const char *name=0);
  ~GrepDialog();
  void  setDirName(const QString &);

signals:
    void itemSelected(const QString &abs_filename, int line);

public slots:
		void slotSearchFor(const QString &pattern);

private slots:
    void templateActivated(int index);
    void childExited();
    void receivedOutput(KProcess *proc, char *buffer, int buflen);
    void itemSelected(const QString&);
    void slotSearch();
    void slotCancel();
    void slotClear();
    void patternTextChanged( const QString &);
private:
    void processOutput();
    void finish();

    QLineEdit *template_edit;
    QComboBox *files_combo, *pattern_combo;
    KURLRequester *dir_combo;
    QCheckBox *recursive_box;
    QListBox *resultbox;
    QPushButton *search_button;
    KProcess *childproc;
    QString buf;
    KConfig* config;
    QStringList lastSearchItems;
    QStringList lastSearchPaths;
};


#endif





