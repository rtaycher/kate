/***************************************************************************
  Copyright:
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
#include <dcopclient.h>
#include <qtimer.h>

// KDE includes.
#include <kaction.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kgenericfactory.h>

// $(kde_includes)/kate
#include <application.h>
#include <documentmanager.h>
#include <document.h>
#include <mainwindow.h>
#include <view.h>
#include <viewmanager.h>

// KateKTTSDPlugin includes.
#include "katekttsd.h"
#include "katekttsd.moc"

class PluginView : public KXMLGUIClient
{
  friend class KatePluginKTTSD;

  public:
    Kate::MainWindow *win;
};

K_EXPORT_COMPONENT_FACTORY( katekttsdplugin, KGenericFactory<KatePluginKTTSD>("katepluginkttsd") )

KatePluginKTTSD::KatePluginKTTSD( QObject* parent, const char* name, const QStringList& )
    : Kate::Plugin ( (Kate::Application*)parent, name )
{
}

KatePluginKTTSD::~KatePluginKTTSD()
{
}

void KatePluginKTTSD::addView(Kate::MainWindow *win)
{
    PluginView *view = new PluginView ();

     (void) new KAction ( i18n("&Speak Text"), "kttsd", 0, this,
                      SLOT( slotReadOut() ), view->actionCollection(),
                      "tools_kttsd" );

    view->setInstance (new KInstance("kate"));
    view->setXMLFile("plugins/katekttsd/ui.rc");
    win->guiFactory()->addClient (view);
    view->win = win;

   m_views.append (view);
}

void KatePluginKTTSD::removeView(Kate::MainWindow *win)
{
  for (uint z=0; z < m_views.count(); z++)
    if (m_views.at(z)->win == win)
    {
      PluginView *view = m_views.at(z);
      m_views.remove (view);
      win->guiFactory()->removeClient (view);
      delete view;
    }
}

void KatePluginKTTSD::slotReadOut()
{
  Kate::View *kv = application()->activeMainWindow()->viewManager()->activeView();

  if (kv)
  {
    KTextEditor::SelectionInterface *si = selectionInterface( kv->document() );
    QString text;

    if ( si->hasSelection() )
      text = si->selection();
    else {
      KTextEditor::EditInterface *ei = editInterface( kv->document() );
      text = ei->text();
    }

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
    if ( !client->call("kttsd", "kspeech", "setText(QString,QString)",
                       data, replyType, replyData, true) )
       QMessageBox::warning( 0, i18n( "DCOP Call Failed" ),
                                 i18n( "The DCOP call setText failed." ));
    QDataStream arg2(data2, IO_WriteOnly);
    
    arg2 << 0;
    if ( !client->call("kttsd", "kspeech", "startText(uint)",
                       data2, replyType, replyData, true) )
       QMessageBox::warning( 0, i18n( "DCOP Call Failed" ),
                                i18n( "The DCOP call startText failed." ));
  }
}

