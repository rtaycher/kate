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
#include "katemainwindow.h"

#include "../interfaces/application.h"

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

KateApp::KateApp (KCmdLineArgs *args)
 : KApplication ()
 , m_args (args)
{
  // Don't handle DCOP requests yet
  dcopClient()->suspend();

  // insert right translations for the katepart
  KGlobal::locale()->insertCatalogue("katepart");

  // some global default
  Kate::Document::setFileChangedDialogsActivated (true);

  // application interface
  m_application = new Kate::Application (this);

  // doc + project man
  m_docManager = new KateDocManager (this);

  // init all normal plugins
  m_pluginManager = new KatePluginManager (this);

  // session manager up
  m_sessionManager = new KateSessionManager (this);

  // application dcop interface
  m_obj = new KateAppDCOPIface (this);

  // real init
  initKate ();
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

KateApp *KateApp::self ()
{
  return (KateApp *) kapp;
}

Kate::Application *KateApp::application ()
{
  return m_application;
}

void KateApp::initKate ()
{
  // handle restore different
  if (isRestored())
  {
    restoreKate ();
  }
  else
  {
    // let us handle our command line args and co ;)
    // we can exit here if session chooser decides
    if (!startupKate ())
    {
      quit ();
      return ;
    }
  }

  // Ok. We are ready for DCOP requests.
  dcopClient()->resume();
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

bool KateApp::startupKate ()
{
  // user specified session to open
  if (m_args->isSet ("start-session"))
  {
    sessionManager()->activateSession (sessionManager()->giveSession (m_args->getOption("start-session")), false, false);
  }
  else if (!m_args->isSet( "stdin" ) && (m_args->count() == 0)) // only start session if no files specified
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

  QTextCodec *codec = m_args->isSet("encoding") ? QTextCodec::codecForName(m_args->getOption("encoding")) : 0;

  Kate::Document::setOpenErrorDialogsActivated (false);
  uint id = 0;
  for (int z=0; z<m_args->count(); z++)
  {
    // this file is no local dir, open it, else warn
    bool noDir = !m_args->url(z).isLocalFile() || !QDir (m_args->url(z).path()).exists();

    if (noDir)
    {
      // open a normal file
      if (codec)
        id = activeMainWindow()->viewManager()->openURL( m_args->url(z), codec->name(), false );
      else
        id = activeMainWindow()->viewManager()->openURL( m_args->url(z), QString::null, false );
    }
    else
      KMessageBox::sorry( activeMainWindow(),
                          i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.").arg(m_args->url(z).url()) );
  }

  Kate::Document::setOpenErrorDialogsActivated (true);

  // handle stdin input
  if( m_args->isSet( "stdin" ) )
  {
    QTextIStream input(stdin);

    // set chosen codec
    if (codec)
      input.setCodec (codec);

    QString line;
    QString text;

    do
    {
      line = input.readLine();
      text.append( line + "\n" );
    } while( !line.isNull() );

    openInput (text);
  }
  else if ( id )
    activeMainWindow()->viewManager()->activateView( id );

  if ( activeMainWindow()->viewManager()->viewCount () == 0 )
    activeMainWindow()->viewManager()->activateView(m_docManager->firstDocument()->documentNumber());

  int line = 0;
  int column = 0;
  bool nav = false;

  if (m_args->isSet ("line"))
  {
    line = m_args->getOption ("line").toInt();
    nav = true;
  }

  if (m_args->isSet ("column"))
  {
    column = m_args->getOption ("column").toInt();
    nav = true;
  }

  if (nav)
    activeMainWindow()->viewManager()->activeView ()->setCursorPosition (line, column);

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
    delete m_mainWindows[0];

  quit ();
}

KatePluginManager *KateApp::pluginManager()
{
  return m_pluginManager;
}

KateDocManager *KateApp::documentManager ()
{
  return m_docManager;
}

KateSessionManager *KateApp::sessionManager ()
{
  return m_sessionManager;
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
      mainWindow->viewManager()->openURL( url, codec->name());
    else
      mainWindow->viewManager()->openURL( url, QString::null );
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

  mainWindow->viewManager()->activeView ()->setCursorPosition (line, column);

  return true;
}

bool KateApp::openInput (const QString &text)
{
  activeMainWindow()->viewManager()->openURL( "", "", true );

  if (!activeMainWindow()->viewManager()->activeView ())
    return false;

  activeMainWindow()->viewManager()->activeView ()->getDoc()->setText (text);

  return true;
}

KateMainWindow *KateApp::newMainWindow (KConfig *sconfig, const QString &sgroup)
{
  KateMainWindow *mainWindow = new KateMainWindow (sconfig, sgroup);
  m_mainWindows.push_back (mainWindow);

  if ((mainWindows() > 1) && m_mainWindows[m_mainWindows.count()-2]->viewManager()->activeView())
    mainWindow->viewManager()->activateView ( m_mainWindows[m_mainWindows.count()-2]->viewManager()->activeView()->getDoc()->documentNumber() );
  else if ((mainWindows() > 1) && (m_docManager->documents() > 0))
    mainWindow->viewManager()->activateView ( (m_docManager->document(m_docManager->documents()-1))->documentNumber() );
  else if ((mainWindows() > 1) && (m_docManager->documents() < 1))
    mainWindow->viewManager()->openURL ( KURL() );

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

  int n = m_mainWindows.findIndex ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  return m_mainWindows[n];
}

uint KateApp::mainWindows () const
{
  return m_mainWindows.size();
}

KateMainWindow *KateApp::mainWindow (uint n)
{
  if (n < m_mainWindows.size())
    return m_mainWindows[n];

  return 0;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
