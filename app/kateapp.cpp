/***************************************************************************
                          kateapp.cpp  -  description
                             -------------------
    begin                : Wed Jan 3 2001
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

#include "kateapp.h"
#include "kateapp.moc"

#include "kateIface.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateviewmanager.h"

#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <kconfig.h>
#include <kwin.h>
#include <ktip.h>
#include <kdebug.h>
#include <klibloader.h>

#include <qfile.h>
#include <qtimer.h>

KateApp::KateApp (bool forcedNewProcess, bool oldState) : KUniqueApplication (true,true,true), Kate::InitPluginManager(),m_initPlugin(0),m_doNotInitialize(0)
{       
  m_application = new Kate::Application (this);
              
  if (forcedNewProcess)
  {
    config()->setGroup("KDE");
    config()->writeEntry("MultipleInstances",oldState);
    config()->sync();
  } 
    
  m_firstStart = true;

  m_mainWindows.setAutoDelete (false);
  
  m_docManager = new KateDocManager (this);

  m_pluginManager = new KatePluginManager (this);
  m_pluginManager->loadAllEnabledPlugins ();

  newMainWindow ();

  connect(this, SIGNAL(lastWindowClosed()), SLOT(quit()));

  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  if (args->isSet("initplugin"))
  {
	QString pluginName=args->getOption("initplugin");
	m_initURL=args->url(0);
	m_initPlugin= static_cast<Kate::InitPlugin*>(Kate::createPlugin (QFile::encodeName(pluginName),
				 (Kate::Application *)kapp)->qt_cast("Kate::InitPlugin"));
	m_initPlugin->activate(args->url(0));
	m_doNotInitialize=m_initPlugin->actionsKateShouldNotPerformOnRealStartup();;
        kdDebug()<<"********************loading init plugin in app constructor"<<endl;
  } else kdDebug()<<"************************no plugin specified"<<endl;

  processEvents();

  if ( isRestored() && KMainWindow::canBeRestored(1) )
    m_mainWindows.first()->restore( true );
  else
    m_mainWindows.first()->restore( false );

  KTipDialog::showTip(m_mainWindows.first());
}

KateApp::~KateApp ()
{
  m_pluginManager->writeConfig ();
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
	if (( m_oldInitLib==QString::null) || (m_oldInitLib!=m_initLib))
	{
		if (m_initPlugin) delete m_initPlugin;
		m_initPlugin=0;
		if (m_oldInitLib!=QString::null) KLibLoader::self()->unloadLibrary(m_oldInitLib.latin1());

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
 
  raiseCurrentMainWindow ();

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
    for (int z=0; z<args->count(); z++)
    {
      m_mainWindows.first()->kateViewManager()->openURL( args->url(z) );
    }
 
    if ( m_mainWindows.first()->kateViewManager()->viewCount () == 0 )
      m_mainWindows.first()->kateViewManager()->openURL( KURL() );     
    
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
  KateMainWindow *mainWindow = new KateMainWindow (m_docManager, m_pluginManager);
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

 // if (m_mainWindows.count() == 0)
   // quit();
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

