
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kateapp.h"
#include "kateapp.moc"

#include "katedocmanager.h"
#include "katepartmanager.h"
#include "katepluginmanager.h"
#include "kateviewmanager.h"
#include "kateappIface.h"
#include "katesession.h"
#include "katemainwindow.h"

#include "../interfaces/application.h"

#include <kdeversion.h>
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

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

KateApp::KateApp (KCmdLineArgs *args)
 : KApplication ()
 , m_shouldExit(false)
 , m_args (args)
{
  // Don't handle DCOP requests yet
  dcopClient()->suspend();

  // insert right translations for the katepart
  KGlobal::locale()->insertCatalog("katepart");
  setQuitOnLastWindowClosed (false);

  // application interface
  m_application = new Kate::Application (this);

  // doc man
  m_docManager = new KateDocManager (this);

  // part man
  m_partManager = new KatePartManager (this);

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

/**
 * Has always been the Kate Versioning Scheme:
 * KDE X.Y.Z contains Kate X-1.Y.Z
 */
QString KateApp::kateVersion (bool fullVersion)
{
  return fullVersion ? QString ("%1.%2.%3").arg(KDE::versionMajor() - 1).arg(KDE::versionMinor()).arg(KDE::versionRelease())
           : QString ("%1.%2").arg(KDE::versionMajor() - 1).arg(KDE::versionMinor());
}

void KateApp::initKate ()
{
 
  kDebug()<<"Setting KATE_PID: '"<<getpid()<<"'"<<endl;
  ::setenv( "KATE_PID", QString("%1").arg(getpid()).latin1(), 1 );

  // handle restore different
  if (isSessionRestored())
  {
    restoreKate ();
  }
  else
  {
    // let us handle our command line args and co ;)
    // we can exit here if session chooser decides
    if (!startupKate ())
    {
      kDebug()<<"startupKate returned false"<<endl;
      m_shouldExit=true;
      return ;
    }
  }

  // Ok. We are ready for DCOP requests.
  dcopClient()->resume();
}

void KateApp::restoreKate ()
{
  // restore the nice files ;) we need it
#ifdef __GNUC__
  #warning fixme later
#endif
  //KTextEditor::Document::setOpenErrorDialogsActivated (false);

  // activate again correct session!!!
  sessionConfig()->setGroup("General");
  QString lastSession (sessionConfig()->readEntry ("Last Session", "default.katesession"));
  sessionManager()->activateSession (KSharedPtr<KateSession>(new KateSession (sessionManager(), lastSession)), false, false, false);

  m_docManager->restoreDocumentList (sessionConfig());

#ifdef __GNUC__
#warning fixme later
#endif
  //KTextEditor::Document::setOpenErrorDialogsActivated (true);

  // restore all windows ;)
  for (int n=1; KMainWindow::canBeRestored(n); n++)
    newMainWindow(sessionConfig(), QString ("%1").arg(n));

  // oh, no mainwindow, create one, should not happen, but make sure ;)
  if (mainWindows() == 0)
    newMainWindow ();

#ifndef Q_WS_WIN
  // notify about start
  KStartupInfo::setNewStartupId( activeMainWindow(), startupId());
#endif
}

bool KateApp::startupKate ()
{
  // user specified session to open
  if (m_args->isSet ("start"))
  {
    sessionManager()->activateSession (sessionManager()->giveSession (m_args->getOption("start")), false, false);
  }
  else if (!m_args->isSet( "stdin" ) && (m_args->count() == 0)) // only start session if no files specified
  {
    // let the user choose session if possible
#if 1
    if (!sessionManager()->chooseSession ())
    {
      kDebug()<<"chooseSession returned false, exiting"<<endl;
      // we will exit kate now, notify the rest of the world we are done
#ifndef Q_WS_WIN
      KStartupInfo::appStarted (startupId());
#endif
      return false;
    }
#else
  QMessageBox::information ( 0, "TEST", "TEST");
#endif

  }

  // oh, no mainwindow, create one, should not happen, but make sure ;)
  if (mainWindows() == 0)
    newMainWindow ();

  // notify about start
#ifndef Q_WS_WIN
  KStartupInfo::setNewStartupId( activeMainWindow(), startupId());
#endif
  QTextCodec *codec = m_args->isSet("encoding") ? QTextCodec::codecForName(m_args->getOption("encoding")) : 0;

  bool tempfileSet = KCmdLineArgs::isTempFileSet();

#ifdef __GNUC__
#warning fixme later
#endif
  //KTextEditor::Document::setOpenErrorDialogsActivated (false);

  KTextEditor::Document *doc = 0;
  for (int z=0; z<m_args->count(); z++)
  {
    // this file is no local dir, open it, else warn
    bool noDir = !m_args->url(z).isLocalFile() || !QDir (m_args->url(z).path()).exists();

    if (noDir)
    {
      // open a normal file
      if (codec)
        doc = activeMainWindow()->viewManager()->openURL( m_args->url(z), codec->name(), false,tempfileSet);
      else
        doc = activeMainWindow()->viewManager()->openURL( m_args->url(z), QString(), false,tempfileSet);
    }
    else
      KMessageBox::sorry( activeMainWindow(),
                          i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.").arg(m_args->url(z).url()) );
  }
#ifdef __GNUC__
#warning fixme later
#endif
  //KTextEditor::Document::setOpenErrorDialogsActivated (true);

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
  else if ( doc )
    activeMainWindow()->viewManager()->activateView( doc );

  if ( activeMainWindow()->viewManager()->viewCount () == 0 )
    activeMainWindow()->viewManager()->activateView(m_docManager->document (0));

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

  if (nav && activeMainWindow()->viewManager()->activeView ())
    activeMainWindow()->viewManager()->activeView ()->setCursorPosition (KTextEditor::Cursor (line, column));

  // show the nice tips
  KTipDialog::showTip(activeMainWindow());

  kDebug()<<"KateApplication::init finished successfull"<<endl;
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

KatePartManager *KateApp::partManager ()
{
  return m_partManager;
}

KateSessionManager *KateApp::sessionManager ()
{
  return m_sessionManager;
}

bool KateApp::openURL (const KUrl &url, const QString &encoding, bool isTempFile)
{
  KateMainWindow *mainWindow = activeMainWindow ();

  if (!mainWindow)
    return false;

  QTextCodec *codec = encoding.isEmpty() ? 0 : QTextCodec::codecForName(encoding.toLatin1());

  kDebug () << "OPEN URL "<< encoding << endl;

  // this file is no local dir, open it, else warn
  bool noDir = !url.isLocalFile() || !QDir (url.path()).exists();

  if (noDir)
  {
    // open a normal file
    if (codec)
      mainWindow->viewManager()->openURL( url, codec->name(),true, isTempFile);
    else
      mainWindow->viewManager()->openURL( url, QString(),true,isTempFile );
  }
  else
    KMessageBox::sorry( mainWindow,
                        i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.").arg(url.url()) );

  return true;
}

bool KateApp::setCursor (int line, int column)
{
  KateMainWindow *mainWindow = activeMainWindow ();

  if (!mainWindow)
    return false;

  if (mainWindow->viewManager()->activeView ())
    mainWindow->viewManager()->activeView ()->setCursorPosition (KTextEditor::Cursor (line, column));

  return true;
}

bool KateApp::openInput (const QString &text)
{
  activeMainWindow()->viewManager()->openURL( KUrl(), "", true );

  if (!activeMainWindow()->viewManager()->activeView ())
    return false;

  KTextEditor::Document *doc = activeMainWindow()->viewManager()->activeView ()->document();

  if (!doc)
    return false;

  return doc->setText (text);
}

KateMainWindow *KateApp::newMainWindow (KConfig *sconfig, const QString &sgroup)
{
  KateMainWindow *mainWindow = new KateMainWindow (sconfig, sgroup);
  m_mainWindows.push_back (mainWindow);

  if ((mainWindows() > 1) && m_mainWindows[m_mainWindows.count()-2]->viewManager()->activeView())
    mainWindow->viewManager()->activateView ( m_mainWindows[m_mainWindows.count()-2]->viewManager()->activeView()->document() );
  else if ((mainWindows() > 1) && (m_docManager->documents() > 0))
    mainWindow->viewManager()->activateView ( (m_docManager->document(m_docManager->documents()-1)) );
  else if ((mainWindows() > 1) && (m_docManager->documents() < 1))
    mainWindow->viewManager()->openURL ( KUrl() );

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

  int n = m_mainWindows.indexOf ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  return m_mainWindows[n];
}

int KateApp::mainWindows () const
{
  return m_mainWindows.size();
}

int KateApp::mainWindowID(KateMainWindow *window) {
  for (int i=0;i<m_mainWindows.size();i++)
    if (window==m_mainWindows[i]) return i;
  return -1;
}

KateMainWindow *KateApp::mainWindow (int n)
{
  if (n < m_mainWindows.size())
    return m_mainWindows[n];

  return 0;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
