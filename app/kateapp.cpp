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

KateApp::KateApp (bool forcedNewProcess, bool oldState)
 : KUniqueApplication (true,true,true)
 , m_firstStart (true)
 , m_initPlugin (0)
 , m_doNotInitialize (0)
{
  // session manager up
  m_sessionManager = new KateSessionManager (this);

  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  // we need to call that now, don't ask me, in the first newInstance run it is wrong !
  if (isRestored())
  {
    m_sessionConfig = sessionConfig ();
  }

  // Don't handle DCOP requests yet
  kapp->dcopClient()->suspend();

  m_mainWindows.setAutoDelete (false);

  // uh, we have forced this session to come up via changing config
  // change it back now
  if (forcedNewProcess)
  {
    config()->setGroup("KDE");
    config()->writeEntry("MultipleInstances",oldState);
    config()->sync();
  }

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

int KateApp::newInstance()
{
  // get our command line arguments ;)
  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  // this will ensure some stuff is only done on real application start
  if (m_firstStart)
  {
    // we restore our great stuff here now ;) super
    if ( restoringSession() )
    {
      // restore the nice files ;) we need it
      Kate::Document::setOpenErrorDialogsActivated (false);

      // use config given by session manager
      if (sessionConfig())
        m_docManager->restoreDocumentList (sessionConfig());

      Kate::Document::setOpenErrorDialogsActivated (true);

      for (int n=1; KMainWindow::canBeRestored(n); n++)
      {
        KateMainWindow *win=newMainWindow(false);
        win->restore ( n, true );
      }
    }
    else
    {
      // user specified session to open
      if (args->isSet ("start-session"))
      {
        kateSessionManager()->activateSession (kateSessionManager()->giveSession (args->getOption("start-session")), false, false);
      }
      else
      {
        // let the user choose session if possible
        kateSessionManager()->chooseSession ();
      }
    }
  }

  // oh, no mainwindow, create one, should not happen, but make sure ;)
  if (mainWindows() == 0)
    newMainWindow ();

  // search the right main window
  // or create one if no window on the current desktop
  KateMainWindow *win = 0;

  if (!m_firstStart && args->isSet ("w"))
  {
    win = newMainWindow ();
  }
  else
  {
    if (activeKateMainWindow() && KWin::windowInfo (activeKateMainWindow()->winId()).isOnCurrentDesktop())
    {
      win = activeKateMainWindow();
    }
    else
    {
      for (uint i=0; i < m_mainWindows.count(); i++)
      {
        if (KWin::windowInfo (m_mainWindows.at(i)->winId()).isOnCurrentDesktop())
        {
          win = m_mainWindows.at(i);
          break;
        }
      }
    }

    // create window on current desktop
    if (!win)
      win = newMainWindow ();
  }

  // raise the window if not first start
  if (!m_firstStart)
  {
    win->raise();
    KWin::activateWindow (win->winId());
  }

  if (m_firstStart && m_initPlugin)
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
          id = m_mainWindows.first()->kateViewManager()->openURL( args->url(z), codec->name(), false );
        else
          id = m_mainWindows.first()->kateViewManager()->openURL( args->url(z), QString::null, false );
      }
      else
        KMessageBox::sorry( m_mainWindows.first(),
                            i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.").arg(args->url(z).url()) );
    }

    if ( id )
      m_mainWindows.first()->kateViewManager()->activateView( id );

    Kate::Document::setOpenErrorDialogsActivated (true);

    if ( m_mainWindows.first()->kateViewManager()->viewCount () == 0 )
      m_mainWindows.first()->kateViewManager()->activateView(m_docManager->firstDocument()->documentNumber());

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
      m_mainWindows.first()->kateViewManager()->activeView ()->setCursorPosition (line, column);
  }

  KStartupInfo::setNewStartupId( m_mainWindows.first(), kapp->startupId());

  if (m_firstStart)
  {
    // very important :)
    m_firstStart = false;

    // show the nice tips
    KTipDialog::showTip(m_mainWindows.first());
  }

  return 0;
}

KateMainWindow *KateApp::newMainWindow ()
{
  return newMainWindow (true);
}

KateMainWindow *KateApp::newMainWindow (bool visible)
{
  KateMainWindow *mainWindow = new KateMainWindow ();
  m_mainWindows.append (mainWindow);

  if ((mainWindows() > 1) && m_mainWindows.at(m_mainWindows.count()-2)->kateViewManager()->activeView())
    mainWindow->kateViewManager()->activateView ( m_mainWindows.at(m_mainWindows.count()-2)->kateViewManager()->activeView()->getDoc()->documentNumber() );
  else if ((mainWindows() > 1) && (m_docManager->documents() > 0))
    mainWindow->kateViewManager()->activateView ( (m_docManager->document(m_docManager->documents()-1))->documentNumber() );
  else if ((mainWindows() > 1) && (m_docManager->documents() < 1))
    mainWindow->kateViewManager()->openURL ( KURL() );

  if (visible)
    mainWindow->show ();


  if (!m_firstStart)
  {
    mainWindow->raise();
    KWin::activateWindow (mainWindow->winId());
  }

  return mainWindow;
}

void KateApp::removeMainWindow (KateMainWindow *mainWindow)
{
  m_mainWindows.remove (mainWindow);
}

void KateApp::openURL (const QString &name)
{
  int n = m_mainWindows.find ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  m_mainWindows.at(n)->kateViewManager()->openURL (KURL(name));

  if ( ! m_firstStart )
  {
    m_mainWindows.at(n)->raise();
    KWin::activateWindow (m_mainWindows.at(n)->winId());
  }
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
