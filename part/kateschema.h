/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __kate_schema_h__
#define __kate_schema_h__

#include "../interfaces/document.h"

#include <qstringlist.h>

#include <kconfig.h>

class KColorButton;

class KateSchemaManager
{
  public:
    KateSchemaManager ();
    ~KateSchemaManager ();

    /**
     * Schema Config changed, update all renderers
     */
    void update (bool readfromfile = true);

    /**
     * return kconfig with right group set or set to Normal if not there
     */
    KConfig *schema (uint number);

    void addSchema (const QString &t);

    /**
     * if not found, defaults to 0
     */
    uint number (const QString &name);

    /**
     * group names in the end, no i18n involved
     */
    QString name (uint number);

    /**
     * Don't modify, list with the names of the schemas (i18n name for the default ones)
     */
    const QStringList &list () { return m_schemas; }

  private:
    KConfig m_config;
    QStringList m_schemas;
};

//
// DIALOGS
//

class KateSchemaConfigColorTab : public QWidget
{
  Q_OBJECT

public:

  KateSchemaConfigColorTab( QWidget *parent = 0, const char *name = 0 );
  ~KateSchemaConfigColorTab();

  private:
    KColorButton *m_back;
    KColorButton *m_selected;
    KColorButton *m_current;
    KColorButton *m_bracket;
    KColorButton *m_wwmarker;
    KColorButton *m_iconborder;
    KColorButton *m_tmarker;

  public:
    void readConfig (KConfig *config);
    void writeConfig (KConfig *config);
};

class KateSchemaConfigFontTab : public QWidget
{
  Q_OBJECT

public:

  KateSchemaConfigFontTab( QWidget *parent = 0, const char *name = 0 );
  ~KateSchemaConfigFontTab();

  public:
    void readConfig (KConfig *config);
    void writeConfig (KConfig *config);

	private:
			class KFontChooser *m_fontchooser;
			QFont myFont;

		private slots:
			void slotFontSelected( const QFont &font );
};

class KateSchemaConfigPage : public Kate::ConfigPage
{
  Q_OBJECT

  public:
    KateSchemaConfigPage ( QWidget *parent );
    ~KateSchemaConfigPage ();

  public slots:
    void apply();
    void reload();
    void reset();
    void defaults();

  private slots:
    void update ();
    void deleteSchema ();
    void newSchema ();
    void schemaChanged (int schema);

  private:
    int m_lastSchema;
    class QTabWidget *m_tabWidget;
    class QPushButton *btndel;
    class QComboBox *schemaCombo;
    KateSchemaConfigColorTab *m_colorTab;
		KateSchemaConfigFontTab *m_fontTab;
};

#endif
