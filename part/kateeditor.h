/***************************************************************************
                          katefactory.h  -  description
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

#ifndef __kate_editor_h__
#define __kate_editor_h__

#include "katedocument.h"

class KateEditor : public KateDocument
{
  Q_OBJECT

  public:
    KateEditor (QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name);
    ~KateEditor ();
    
  private:
    KateView *myView;
};

#endif
