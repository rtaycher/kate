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

// Qt includes.
#include <qmessagebox.h>
#include <qstring.h>
#include <qtimer.h>

// KDE includes.
#include <kdebug.h>
#include <kaction.h>
#include <kinstance.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <kgenericfactory.h>

// $(kde-includes/kate)
#include <document.h>

// katepartkttsd includes.
#include "katepartkttsd.h"
#include "katepartkttsd.moc"

K_EXPORT_COMPONENT_FACTORY( libkatepartkttsdplugin, KGenericFactory<KatePartPluginKTTSD>("katepartpluginkttsd") )

KatePartPluginKTTSD::KatePartPluginKTTSD( QObject* parent, const char* name, const QStringList& )
    : Plugin( parent, name )
{
    (void) new KAction( "&Speak Text",
                        "kttsd", 0,
                        this, SLOT(slotReadOut()),
                        actionCollection(), "tools_kttsd" );
}

KatePartPluginKTTSD::~KatePartPluginKTTSD()
{
}

void KatePartPluginKTTSD::slotReadOut()
{
    // The parent is assumed to be a KHTMLPart
    if ( !parent()->inherits("Kate::Document") )
       QMessageBox::warning( 0, i18n( "Cannot Read Source" ),
                                i18n( "You cannot read anything except plain texts with\n"
                                      "this plugin, sorry." ));
    else
    {
        Kate::Document *kd = ((Kate::Document *) parent());
        if (kd)
        {
            QString text;

            if ( kd->hasSelection() )
                text = kd->selection();
            else
                text = kd->text();

            DCOPClient *client = kapp->dcopClient();
            // If KTTSD not running, start it.
            if (!client->isApplicationRegistered("kttsd"))
            {
                QString error;
                if (kapp->startServiceByName("KTTSD", QStringList(), &error))
                    QMessageBox::warning(0, i18n( "Starting KTTSD Failed"), error );
                else
                {
                    // Give KTTSD time to load.
                    QTimer::singleShot(1000, this, SLOT(slotReadOut()));
                    return;
                }
            }
            QByteArray  data;
            QByteArray  data2;
            QCString    replyType;
            QByteArray  replyData;
            QDataStream arg(data, IO_WriteOnly);
            arg << text << "";
            if ( !client->call("kttsd", "KSpeech", "setText(QString,QString)",
                               data, replyType, replyData, true) )
               QMessageBox::warning( 0, i18n( "DCOP Call Failed" ),
                                        i18n( "The DCOP call setText failed." ));
            QDataStream arg2(data2, IO_WriteOnly);
            arg2 << 0;
            if ( !client->call("kttsd", "KSpeech", "startText(uint)",
                               data2, replyType, replyData, true) )
               QMessageBox::warning( 0, i18n( "DCOP Call Failed" ),
                                        i18n( "The DCOP call startText failed." ));
        }
    }
}
