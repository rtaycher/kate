/***************************************************************************
                          katemaildialog.h
                          Misc dialogs and dialog pages for Kate
                             -------------------
    begin                : Wed Mar 06 2002
    copyright            : (C) 2002 by Anders Lund
    email                : anders@alweb.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KATE_MAILFILES_DIALOG_H_
#define _KATE_MAILFILES_DIALOG_H_

#include <kate/document.h>

#include <kdialogbase.h>
#include <kurl.h>
#include <qptrlist.h>

class QString;
class QStringList;
class KateMainWindow;

/**
    This is a dialog for choosing which of the open files to mail.
    The current file is selected by default, the dialog can be expanded
    to display all the files if required.
    
*/
class KateMailDialog : public KDialogBase {
  Q_OBJECT
  public:
    KateMailDialog( QWidget *parent=0,
                          KateMainWindow *mainwin=0 );
    ~KateMailDialog() {};

    /**
        @return a list of the selected docs.
    */
    QPtrList<Kate::Document> selectedDocs();
  private slots:
    void slotShowButton();
  private:
    class KListView *list;
    class QLabel *lInfo;
    KateMainWindow *mainWindow;
    class QVBox *mw;

};

#endif // _KATE_MAILFILES_DIALOG_H_
