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
 , m_initPlugin (0)
 , m_doNotInitialize (0)
{
  // session manager up
  m_sessionManager = new KateSessionManager (this);

  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  // Don't handle DCOP requests yet
  kapp->dcopClient()->suspend();

  m_mainWindows.setAutoDelete (false);

  // application interface
  m_application = new Kate::Application (this);

  // init plugin manager
  m_initPluginManager = new Kate::InitPluginManager (this);

  // application dcop interface
  m_obj = new KateAppDCOPIface (this);

  // insert right translations for the katepart
  KGlobal::locale()->insertCatalogue("katepart");

  // doc + project man
  m_docManager = new KateDocManager (this);

  // init all normal plugins
  m_pluginManager = new KatePluginManager (this);

  if (args->isSet("initplugin"))
  {
    QString pluginName=args->getOption("initplugin");

    m_initURL=args->url(0);

    m_initPlugin= static_cast<Kate::InitPlugin*>(Kate::createPlugin (QFile::encodeName(pluginName), (Kate::Application *)kapp)->qt_cast("Kate::InitPlugin"));

    m_initPlugin->activate(args->url(0));

    m_doNotInitialize=m_initPlugin->actionsKateShouldNotPerformOnRealStartup();

//      kdDebug(13001)<<"********************loading init plugin in app constructor"<<endl;
  }

  // Ok. We are ready for DCOP requests.
  kapp->dcopClient()->resume();

  // notify our self on enter the event loop
  QTimer::singleShot(10,this,SLOT(callOnEventLoopEnter()));
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

//   kdDebug(13001)<<"callOnEventLoopEnter(): "<<kapp->loopLevel()<<"*****************************"<<endl;
}

void KateApp::performInit(const QString &libname, const KURL &url)
{
  if (m_initPlugin)
    m_oldInitLib=m_initLib;
  else
    m_oldInitLib=QString::null;

  m_initURL=url;
  m_initLib=libname;

  QTimer::singleShot(0,this,SLOT(performInit()));
}

void KateApp::performInit()
{
  if (( m_oldInitLib.isNull()) || (m_oldInitLib!=m_initLib))
  {
    delete m_initPlugin;
    m_initPlugin=0;

    if (!m_oldInitLib.isNull())
      KLibLoader::self()->unloadLibrary(m_oldInitLib.latin1());

    m_initPlugin = static_cast<Kate::InitPlugin*>(Kate::createPlugin (QFile::encodeName(m_initLib), (Kate::Application *)kapp)->qt_cast("Kate::InitPlugin"));
  }

  m_initPlugin->activate(m_initURL);
  m_initPlugin->initKate();
}

Kate::InitPlugin *KateApp::initPlugin() const
{
  return m_initPlugin;
}

KURL KateApp::initScript() const {return m_initURL;}

void KateApp::restoreKate ()
{
  // restore the nice files ;) we need it
  Kate::Document::setOpenErrorDialogsActivated (false);

  // activate again correct session!!!
  sessionConfig()->setGroup("General");
  QString lastSession (sessionConfig()->readEntry ("Last Session", "default.katesession"));
  kateSessionManager()->activateSession (new KateSession (kateSessionManager(), lastSession, ""), false, false, false);

  m_docManager->restoreDocumentList (sessionConfig());

  Kate::Document::setOpenErrorDialogsActivated (true);

  // restore all windows ;)
  for (int n=1; KMainWindow::canBeRestored(n); n++)
    newMainWindow(sessionConfig(), QString ("%1").arg(n));

  // oh, no mainwindow, create one, should not happen, but make sure ;)
  if (mainWindows() == 0)
    newMainWindow ();

  // notify about start
  KStartupInfo::setNewStartupId( activeKateMainWindow(), kapp->startupId());
}

bool KateApp::startupKate (KCmdLineArgs* args)
{
  // user specified session to open
  if (args->isSet ("start-session"))
  {
    kateSessionManager()->activateSession (kateSessionManager()->giveSession (args->getOption("start-session")), false, false);
  }
  else if (args->count() == 0) // only start session if no files specified
  {
    // let the user choose session if possible
    if (!kateSessionManager()->chooseSession ())
    {
      // we will exit kate now, notify the rest of the world we are done
      KStartupInfo::appStarted (kapp->startupId());
      return false;
    }
  }

  // oh, no mainwindow, create one, should not happen, but make sure ;)
  if (mainWindows() == 0)
    newMainWindow ();

  // notify about start
  KStartupInfo::setNewStartupId( activeKateMainWindow(), kapp->startupId());

  if (m_initPlugin)
  {
    m_initPlugin->initKate();
  }
  else if (args->isSet("initplugin"))
  {
    performInit(args->getOption("initplugin"),args->url(0));
  }
  else
  {
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
          id = activeKateMainWindow()->kateViewManager()->openURL( args->url(z), codec->name(), false );
        else
          id = activeKateMainWindow()->kateViewManager()->openURL( args->url(z), QString::null, false );
      }
      else
        KMessageBox::sorry( activeKateMainWindow(),
                            i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.").arg(args->url(z).url()) );
    }

    if ( id )
      activeKateMainWindow()->kateViewManager()->activateView( id );

    Kate::Document::setOpenErrorDialogsActivated (true);

    if ( activeKateMainWindow()->kateViewManager()->viewCount () == 0 )
      activeKateMainWindow()->kateViewManager()->activateView(m_docManager->firstDocument()->documentNumber());

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
      activeKateMainWindow()->kateViewManager()->activeView ()->setCursorPosition (line, column);
  }

  // show the nice tips
  KTipDialog::showTip(activeKateMainWindow());

  return true;
}

void KateApp::shutdownKate (KateMainWindow *win)
{
  if (!win->queryClose_internal())
    return;

  kateSessionManager()->saveActiveSession(true, true);

  // detach the dcopClient
  kapp->dcopClient()->detach();

  // cu main windows
  while (!m_mainWindows.isEmpty())
    delete m_mainWindows.take (0);

  quit ();
}

bool KateApp::openURL (const KURL &url, const QString &encoding)
{
  KateMainWindow *mainWindow = activeKateMainWindow ();

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

  KateMainWindow *mainWindow = activeKateMainWindow ();

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

KateMainWindow *KateApp::activeKateMainWindow ()
{
  if (m_mainWindows.isEmpty())
    return 0;

  int n = m_mainWindows.find ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  return m_mainWindows.at(n);
}

Kate::MainWindow *KateApp::activeMainWindow ()
{
  if (!activeKateMainWindow())
    return 0;

  return activeKateMainWindow()->mainWindow();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
