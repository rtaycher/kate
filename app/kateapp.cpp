/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

#include "kateapp.h"
#include "kateapp.moc"

#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateviewmanager.h"
#include "kateappIface.h"
#include "katesession.h"

#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <kconfig.h>
#include <kwin.h>
#include <ktip.h>
#include <kdebug.h>
#include <klibloader.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <ksimpleconfig.h>
#include <kstartupinfo.h>

#include <qfile.h>
#include <qtimer.h>
#include <qdir.h>
#include <qtextcodec.h>

KateApp::KateApp ()
 : KApplication ()
{
  // session manager up
  m_sessionManager = new KateSessionManager (this);

  m_mainWindows.setAutoDelete (false);

  // application interface
  m_application = new Kate::Application (this);

  // application dcop interface
  m_obj = new KateAppDCOPIface (this);

  // insert right translations for the katepart
  KGlobal::locale()->insertCatalogue("katepart");

  // doc + project man
  m_docManager = new KateDocManager (this);

  // init all normal plugins
  m_pluginManager = new KatePluginManager (this);

   // notify our self on enter the event loop
  QTimer::singleShot (10, this, SLOT(callOnEventLoopEnter()));
}

KateApp::~KateApp ()
{
  // cu dcop interface
  delete m_obj;

  // cu plugin manager
  delete m_pluginManager;

  // delete this now, or we crash
  delete m_docManager;
}

void KateApp::callOnEventLoopEnter()
{
  emit onEventLoopEnter();
  disconnect(this,SIGNAL(onEventLoopEnter()),0,0);
  emit m_application->onEventLoopEnter();
  disconnect(m_application,SIGNAL(onEventLoopEnter()),0,0);
}

void KateApp::restoreKate ()
{
  // restore the nice files ;) we need it
  Kate::Document::setOpenErrorDialogsActivated (false);

  // activate again correct session!!!
  sessionConfig()->setGroup("General");
  QString lastSession (sessionConfig()->readEntry ("Last Session", "default.katesession"));
  sessionManager()->activateSession (new KateSession (sessionManager(), lastSession, ""), false, false, false);

  m_docManager->restoreDocumentList (sessionConfig());

  Kate::Document::setOpenErrorDialogsActivated (true);

  // restore all windows ;)
  for (int n=1; KMainWindow::canBeRestored(n); n++)
    newMainWindow(sessionConfig(), QString ("%1").arg(n));

  // oh, no mainwindow, create one, should not happen, but make sure ;)
  if (mainWindows() == 0)
    newMainWindow ();

  // notify about start
  KStartupInfo::setNewStartupId( activeMainWindow(), startupId());
}

bool KateApp::startupKate (KCmdLineArgs* args)
{
  // user specified session to open
  if (args->isSet ("start-session"))
  {
    sessionManager()->activateSession (sessionManager()->giveSession (args->getOption("start-session")), false, false);
  }
  else if (args->count() == 0) // only start session if no files specified
  {
    // let the user choose session if possible
    if (!sessionManager()->chooseSession ())
    {
      // we will exit kate now, notify the rest of the world we are done
      KStartupInfo::appStarted (startupId());
      return false;
    }
  }

  // oh, no mainwindow, create one, should not happen, but make sure ;)
  if (mainWindows() == 0)
    newMainWindow ();

  // notify about start
  KStartupInfo::setNewStartupId( activeMainWindow(), startupId());

  QTextCodec *codec = args->isSet("encoding") ? QTextCodec::codecForName(args->getOption("encoding")) : 0;

  Kate::Document::setOpenErrorDialogsActivated (false);
  uint id = 0;
  for (int z=0; z<args->count(); z++)
  {
    // this file is no local dir, open it, else warn
    bool noDir = !args->url(z).isLocalFile() || !QDir (args->url(z).path()).exists();

    if (noDir)
    {
      // open a normal file
      if (codec)
        id = activeMainWindow()->kateViewManager()->openURL( args->url(z), codec->name(), false );
      else
        id = activeMainWindow()->kateViewManager()->openURL( args->url(z), QString::null, false );
    }
    else
      KMessageBox::sorry( activeMainWindow(),
                          i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.").arg(args->url(z).url()) );
  }

  if ( id )
    activeMainWindow()->kateViewManager()->activateView( id );

  Kate::Document::setOpenErrorDialogsActivated (true);

  if ( activeMainWindow()->kateViewManager()->viewCount () == 0 )
    activeMainWindow()->kateViewManager()->activateView(m_docManager->firstDocument()->documentNumber());

  int line = 0;
  int column = 0;
  bool nav = false;

  if (args->isSet ("line"))
  {
    line = args->getOption ("line").toInt();
    nav = true;
  }

  if (args->isSet ("column"))
  {
    column = args->getOption ("column").toInt();
    nav = true;
  }

  if (nav)
    activeMainWindow()->kateViewManager()->activeView ()->setCursorPosition (line, column);

  // show the nice tips
  KTipDialog::showTip(activeMainWindow());

  return true;
}

void KateApp::shutdownKate (KateMainWindow *win)
{
  if (!win->queryClose_internal())
    return;

  sessionManager()->saveActiveSession(true, true);

  // detach the dcopClient
  dcopClient()->detach();

  // cu main windows
  while (!m_mainWindows.isEmpty())
    delete m_mainWindows.take (0);

  quit ();
}

bool KateApp::openURL (const KURL &url, const QString &encoding)
{
  KateMainWindow *mainWindow = activeMainWindow ();

  if (!mainWindow)
    return false;

  QTextCodec *codec = encoding.isEmpty() ? 0 : QTextCodec::codecForName(encoding.latin1());


  kdDebug () << "OPEN URL "<< encoding << endl;

  // this file is no local dir, open it, else warn
  bool noDir = !url.isLocalFile() || !QDir (url.path()).exists();

  if (noDir)
  {
    // open a normal file
    if (codec)
      mainWindow->kateViewManager()->openURL( url, codec->name());
    else
      mainWindow->kateViewManager()->openURL( url, QString::null );
  }
  else
    KMessageBox::sorry( mainWindow,
                        i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.").arg(url.url()) );

  return true;
}

bool KateApp::setCursor (int line, int column)
{
  kdDebug () << "SET CURSOR" << endl;

  KateMainWindow *mainWindow = activeMainWindow ();

  if (!mainWindow)
    return false;

  mainWindow->kateViewManager()->activeView ()->setCursorPosition (line, column);

  return true;
}

KateMainWindow *KateApp::newMainWindow (KConfig *sconfig, const QString &sgroup)
{
  KateMainWindow *mainWindow = new KateMainWindow (sconfig, sgroup);
  m_mainWindows.append (mainWindow);

  if ((mainWindows() > 1) && m_mainWindows.at(m_mainWindows.count()-2)->kateViewManager()->activeView())
    mainWindow->kateViewManager()->activateView ( m_mainWindows.at(m_mainWindows.count()-2)->kateViewManager()->activeView()->getDoc()->documentNumber() );
  else if ((mainWindows() > 1) && (m_docManager->documents() > 0))
    mainWindow->kateViewManager()->activateView ( (m_docManager->document(m_docManager->documents()-1))->documentNumber() );
  else if ((mainWindows() > 1) && (m_docManager->documents() < 1))
    mainWindow->kateViewManager()->openURL ( KURL() );

  mainWindow->show ();

  return mainWindow;
}

void KateApp::removeMainWindow (KateMainWindow *mainWindow)
{
  m_mainWindows.remove (mainWindow);
}

KateMainWindow *KateApp::activeMainWindow ()
{
  if (m_mainWindows.isEmpty())
    return 0;

  int n = m_mainWindows.find ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  return m_mainWindows.at(n);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
