/***************************************************************************
                          kateviewdialog.h  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann 
    email                : cullmann@kde.org
 ***************************************************************************/ 

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KATE_VIEWDIALOG_H_
#define _KATE_VIEWDIALOG_H_

class QCheckBox;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSpinBox;
class KColorButton;
class KIntNumInput;
class KComboBox;
class KRegExpDialog;

#include <kdialogbase.h>
#include "kateview.h"

class SearchDialog : public KDialogBase
{
  Q_OBJECT

  public:
    SearchDialog( QWidget *parent, QStringList &searchFor, QStringList &replaceWith, int flags );
    QString getSearchFor();
    QString getReplaceWith();
    int getFlags();
    void setSearchText( const QString &searchstr );

  protected slots:
    void slotOk();
    void selectedStateChanged (int);
    void slotEditRegExp();

  protected:
    KComboBox *m_search;
    KComboBox *m_replace;
    QCheckBox *m_opt1;
    QCheckBox *m_opt2;
    QCheckBox *m_opt3;
    QCheckBox *m_optRegExp;
    QCheckBox *m_opt4;
    QCheckBox *m_opt5;
    QCheckBox *m_opt6;
    QDialog *m_regExpDialog;
};

class ReplacePrompt : public KDialogBase
{
    Q_OBJECT

  public:

    ReplacePrompt(QWidget *parent);

  signals:

    void clicked();

  protected slots:

    void slotUser1( void ); // All
    void slotUser2( void ); // No
    void slotUser3( void ); // Yes
    virtual void done(int);
};

class GotoLineDialog : public KDialogBase
{
    Q_OBJECT

  public:

    GotoLineDialog(QWidget *parent, int line, int max);
    int getLine();

  protected:

    KIntNumInput *e1;
    QPushButton *btnOK;
};

class IndentConfigTab : public QWidget
{
    Q_OBJECT

  public:

    IndentConfigTab(QWidget *parent, KateDocument *);
    void getData(KateDocument *);

  protected:

    static const int numFlags = 6;
    static const int flags[numFlags];
    QCheckBox *opt[numFlags];
};

class SelectConfigTab : public QWidget
{
    Q_OBJECT

 public:

    SelectConfigTab(QWidget *parent, KateDocument *);
    void getData(KateDocument *);

  protected:

    static const int numFlags = 5;
    static const int flags[numFlags];
    QCheckBox *opt[numFlags];
};

class EditConfigTab : public QWidget
{
    Q_OBJECT

  public:

    EditConfigTab(QWidget *parent, KateDocument *);
    void getData(KateDocument *);

  protected:

    static const int numFlags = 9;
    static const int flags[numFlags];
    QCheckBox *opt[numFlags];

    KIntNumInput *e1;
    KIntNumInput *e2;
    KIntNumInput *e3;
};

class ColorConfig : public QWidget
{
  Q_OBJECT

public:

  ColorConfig( QWidget *parent = 0, char *name = 0 );
  ~ColorConfig();

  void setColors( QColor * );
  void getColors( QColor * );

private:

  KColorButton *m_back;
  KColorButton *m_selected;
};

class FontConfig : public QWidget
{
  Q_OBJECT

public:

  FontConfig( QWidget *parent = 0, char *name = 0 );
  ~FontConfig();

  void setFont ( const QFont &font );
  QFont getFont ( ) { return myFont; };

  private:
    class KFontChooser *m_fontchooser;
    QFont myFont;

  private slots:
    void slotFontSelected( const QFont &font );
};


#endif
