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


#include "katekttsd.h"
#include "katekttsd.moc"

#include <kaction.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <qmessagebox.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <qtimer.h>

// $(kde_includes)/kate
#include <application.h>
#include <documentmanager.h>
#include <document.h>
#include <mainwindow.h>
#include <plugin.h>
#include <view.h>
#include <viewmanager.h>

class PluginView : public KXMLGUIClient
{
  friend class KatePluginKTTSD;

  public:
    Kate::MainWindow *win;
};

extern "C"
{
  void* init_katekttsdplugin()
  {
    KGlobal::locale()->insertCatalogue("katekttsd");
    return new KatePluginFactory;
  }
}

KatePluginFactory::KatePluginFactory()
{
  s_instance = new KInstance( "kate" );
}

KatePluginFactory::~KatePluginFactory()
{
  delete s_instance;
}

QObject* KatePluginFactory::createObject( QObject* parent, const char* name, const char*, const QStringList & )
{
  return new KatePluginKTTSD( parent, name );
}

KInstance* KatePluginFactory::s_instance = 0L;

KatePluginKTTSD::KatePluginKTTSD( QObject* parent, const char* name )
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
            QMessageBox::warning(0, i18n( "Starting KTTSD failed"), error );
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
       QMessageBox::warning( 0, i18n( "DCOP Call failed" ),
                                 i18n( "The DCOP call setText failed" ));
    QDataStream arg2(data2, IO_WriteOnly);
    arg2 << 0;
    if ( !client->call("kttsd", "kspeech", "startText(uint)",
                       data2, replyType, replyData, true) )
       QMessageBox::warning( 0, i18n( "DCOP Call failed" ),
                                i18n( "The DCOP call startText failed" ));
  }
}
