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

// $Id$

#include "kateapp.h"
#include "kateapp.moc"

#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateviewmanager.h"
#include "kateappIface.h"

#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <kconfig.h>
#include <kwin.h>
#include <ktip.h>
#include <kdebug.h>
#include <klibloader.h>
#include <klocale.h>
#include <ksimpleconfig.h>

#include <kio/netaccess.h>

#include <qfile.h>
#include <qtimer.h>

KateApp::KateApp (bool forcedNewProcess, bool oldState) : KUniqueApplication (true,true,true),m_initPlugin(0),m_doNotInitialize(0)
{
  m_application = new Kate::Application (this);
  m_initPluginManager = new Kate::InitPluginManager (this);

  m_obj = new KateAppDCOPIface (this);

  KGlobal::locale()->insertCatalogue("katepart");

  if (forcedNewProcess)
  {
    config()->setGroup("KDE");
    config()->writeEntry("MultipleInstances",oldState);
    config()->sync();
  }

  m_firstStart = true;
  kapp->dcopClient()->suspend(); // Don't handle DCOP requests yet

  m_mainWindows.setAutoDelete (false);

  m_docManager = new KateDocManager (this);

  m_projectManager = new KateProjectManager (this);

  m_pluginManager = new KatePluginManager (this);
  m_pluginManager->loadAllEnabledPlugins ();

  // first be sure we have at least one window
  KateMainWindow *win = newMainWindow ();

  // we restore our great stuff here now ;) super
  if ( isRestored() )
  {
    // restore the nice projects & files ;) we need it
    m_projectManager->restoreProjectList (sessionConfig());
    m_docManager->restoreDocumentList (sessionConfig());

    // window config
    win->restoreWindowConfiguration (sessionConfig());
  }
  else
  {
    config()->setGroup("General");

    KSimpleConfig* scfg = new KSimpleConfig("katesessionrc", false);

    // restore our nice projects if wanted
    if (config()->readBoolEntry("Restore Projects", false))
      m_projectManager->restoreProjectList (scfg);

    // reopen our nice files if wanted
    if (config()->readBoolEntry("Restore Documents", false))
      m_docManager->restoreDocumentList (scfg);

    // window config
    if (config()->readBoolEntry("Restore Window Configuration", false))
      win->restoreWindowConfiguration (scfg);

    delete scfg;
  }

  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  if (args->isSet("initplugin"))
  {
	QString pluginName=args->getOption("initplugin");
	m_initURL=args->url(0);
	m_initPlugin= static_cast<Kate::InitPlugin*>(Kate::createPlugin (QFile::encodeName(pluginName),
				 (Kate::Application *)kapp)->qt_cast("Kate::InitPlugin"));
	m_initPlugin->activate(args->url(0));
	m_doNotInitialize=m_initPlugin->actionsKateShouldNotPerformOnRealStartup();
        kdDebug()<<"********************loading init plugin in app constructor"<<endl;
  } else kdDebug()<<"************************no plugin specified"<<endl;

  processEvents();


  KTipDialog::showTip(m_mainWindows.first());

  kapp->dcopClient()->resume(); // Ok. We are ready for DCOP requests.

  QTimer::singleShot(10,this,SLOT(callOnEventLoopEnter()));
}

KateApp::~KateApp ()
{
  m_pluginManager->writeConfig ();

  delete m_obj;
}

void KateApp::callOnEventLoopEnter()
{
  emit onEventLoopEnter();
  disconnect(this,SIGNAL(onEventLoopEnter()),0,0);
  emit m_application->onEventLoopEnter();
  disconnect(m_application,SIGNAL(onEventLoopEnter()),0,0);

  kdDebug()<<"callOnEventLoopEnter(): "<<kapp->loopLevel()<<"*****************************"<<endl;
}

void KateApp::performInit(const QString &libname, const KURL &url)
{
	if (m_initPlugin) m_oldInitLib=m_initLib; else m_oldInitLib=QString::null;
	m_initURL=url;
	m_initLib=libname;
	QTimer::singleShot(0,this,SLOT(performInit()));
}

void KateApp::performInit()
{
	if (( m_oldInitLib.isNull()) || (m_oldInitLib!=m_initLib))
	{
		if (m_initPlugin) delete m_initPlugin;
		m_initPlugin=0;
		if (!m_oldInitLib.isNull()) KLibLoader::self()->unloadLibrary(m_oldInitLib.latin1());

        	m_initPlugin= static_cast<Kate::InitPlugin*>(Kate::createPlugin (QFile::encodeName(m_initLib),
                                 (Kate::Application *)kapp)->qt_cast("Kate::InitPlugin"));
	}
        m_initPlugin->activate(m_initURL);
	m_initPlugin->initKate();
}

Kate::InitPlugin *KateApp::initPlugin() const {return m_initPlugin;}

KURL KateApp::initScript() const {return m_initURL;}

int KateApp::newInstance()
{
  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  if (!m_firstStart && args->isSet ("w"))
    newMainWindow ();

  if (!m_firstStart)
    raiseCurrentMainWindow ();

  kdDebug()<<"******************************************** loop depth"<<kapp->loopLevel()<<endl;

  if (m_firstStart && m_initPlugin)
  {
	m_initPlugin->initKate();
        kdDebug()<<"***************************** INIT PLUGIN ON FIRST START"<<endl;
  }
  else if (args->isSet("initplugin"))
  {
        kdDebug()<<"***************************** INIT PLUGIN ON ANY  START"<<endl;
	performInit(args->getOption("initplugin"),args->url(0));

  }
  else
  {
    Kate::Document::setOpenErrorDialogsActivated (false);
    for (int z=0; z<args->count(); z++)
    {
      if (!KIO::NetAccess::mimetype( args->url(z), m_mainWindows.first() ).startsWith(QString ("inode/directory")))
        m_mainWindows.first()->kateViewManager()->openURL( args->url(z) );
    }
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
  m_firstStart = false;

  return 0;
}

KateMainWindow *KateApp::newMainWindow ()
{
  KateMainWindow *mainWindow = new KateMainWindow (m_docManager, m_pluginManager, m_projectManager);
  m_mainWindows.append (mainWindow);

  if ((mainWindows() > 1) && m_mainWindows.at(m_mainWindows.count()-2)->kateViewManager()->activeView())
    mainWindow->kateViewManager()->activateView ( m_mainWindows.at(m_mainWindows.count()-2)->kateViewManager()->activeView()->getDoc()->documentNumber() );
  else if ((mainWindows() > 1) && (m_docManager->documents() > 0))
    mainWindow->kateViewManager()->activateView ( (m_docManager->document(m_docManager->documents()-1))->documentNumber() );
  else if ((mainWindows() > 1) && (m_docManager->documents() < 1))
    mainWindow->kateViewManager()->openURL ( KURL() );

  mainWindow->show ();
  mainWindow->raise();
  KWin::setActiveWindow (mainWindow->winId());

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

  m_mainWindows.at(n)->raise();
  KWin::setActiveWindow (m_mainWindows.at(n)->winId());
}

void KateApp::raiseCurrentMainWindow ()
{
  int n = m_mainWindows.find ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  m_mainWindows.at(n)->raise();
  KWin::setActiveWindow (m_mainWindows.at(n)->winId());
}

Kate::MainWindow *KateApp::activeMainWindow ()
{
  int n = m_mainWindows.find ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  return m_mainWindows.at(n)->mainWindow();
}

KateMainWindow *KateApp::activeKateMainWindow ()
{
  int n = m_mainWindows.find ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  return m_mainWindows.at(n);
}
