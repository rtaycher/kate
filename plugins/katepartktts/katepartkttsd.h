/***************************************************************************
  Copyright:
  (C) 2002 by George Russell <george.russell@clara.net>
  (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KATEPARTKTTSD_H
#define KATEPARTKTTSD_H

#include <kparts/plugin.h>
// #include <klibloader.h>

class KURL;
class KInstance;

/**
 * A plugin is the way to add actions to an existing @ref KParts application,
 * or to a @ref Part.
 *
 * The XML of those plugins looks exactly like of the shell or parts,
 * with one small difference: The document tag should have an additional
 * attribute, named "library", and contain the name of the library implementing
 * the plugin.
 *
 * If you want this plugin to be used by a part, you need to
 * install the rc file under the directory
 * "data" (KDEDIR/share/apps usually)+"/instancename/kpartplugins/"
 * where instancename is the name of the part's instance.
 **/
class KatePartPluginKTTSD : public KParts::Plugin
{
    Q_OBJECT
public:

    /**
     * Construct a new KParts plugin.
     */
    KatePartPluginKTTSD( QObject* parent = 0, const char* name = 0, const QStringList& = QStringList() );

    /**
     * Destructor.
     */
    virtual ~KatePartPluginKTTSD();
public slots:
    void slotReadOut();
};

#endif
