/***************************************************************************
                          kantpartdocument.h  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : crossfire@babylon2k.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KANTPARTDOCUMENT_H
#define KANTPARTDOCUMENT_H

#include "../kantmain.h"

#include "../document/kantdocument.h"

class KantPartDocument : public KantDocument
{
  friend class KantPartView;

  public:
    KantPartDocument (bool bSingleViewMode = false, bool bBrowserView = false, QWidget *parentWidget = 0, const char *widgetName = 0, QObject *parent = 0, const char *name = 0);
    ~KantPartDocument ();

    KTextEditor::View *createView( QWidget *parent, const char *name );
};

class KantPartBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT

  public:
    KantPartBrowserExtension( KantPartDocument *doc );

  private slots:
    void copy();
    void slotSelectionChanged();

  private:
    KantPartDocument *m_doc;
};

#endif
