/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kateconsole.h"
#include "kateconsole.moc"

#include <kiconloader.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <kde_terminal_interface.h>

#include <kparts/part.h>
#include <kaction.h>

#include <kurl.h>
#include <klibloader.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kapplication.h>
//Added by qt3to4:
#include <QShowEvent>

#include <kgenericfactory.h>

K_EXPORT_COMPONENT_FACTORY( katekonsoleplugin, KGenericFactory<Kate::Private::Plugin::KateKonsolePlugin>( "katekonsoleplugin" ) )

using namespace Kate::Private::Plugin;

KateKonsolePlugin::KateKonsolePlugin( QObject* parent, const char* name, const QStringList&):
  Kate::Plugin ( (Kate::Application*)parent, name ) {
  if (!kapp->authorize("shell_access")) {
    KMessageBox::sorry(0, i18n ("You don't have enough karma to access a shell or terminal emulation"));
  }
}

void KateKonsolePlugin::addView(Kate::MainWindow *win) {
  kdDebug()<<"KateKonsolePlugin::createView"<<endl;
  // ONLY ALLOW SHELL ACCESS IF ALLOWED ;)
  if (kapp->authorize("shell_access")) {
    kdDebug()<<"After auth check"<<endl;
    QWidget *toolview=win->createToolView ("kate_private_plugin_katekonsoleplugin", MainWindow::Bottom, SmallIcon("konsole"), i18n("Terminal"));
    m_views.append(new KateConsole(win,toolview));
  }
}

void KateKonsolePlugin::removeView(Kate::MainWindow *win) {
  for(QLinkedList<KateConsole*>::iterator it=m_views.begin();it!=m_views.end();++it) {
    if ((*it)->mainWindow()==win) {
      QWidget *pw=(*it)->parentWidget();
      delete *it;
      delete pw;
      m_views.erase(it);
      break;
    }
  }
}

KateConsole::KateConsole (Kate::MainWindow *mw, QWidget *parent)
 : Q3VBox (parent), KXMLGUIClient()
 , m_part (0)
 , m_mw (mw)
 , m_toolView (parent)
{
    new KAction(i18n("&Pipe to Console"), "pipe", 0, this, SLOT(slotPipeToConsole()), actionCollection(), "katekonsole_tools_pipe_to_terminal");
    setInstance (new KInstance("kate"));
    setXMLFile("plugins/katekonsole/ui.rc");
    m_mw->guiFactory()->addClient (this);

}

KateConsole::~KateConsole ()
{
  m_mw->guiFactory()->removeClient (this);
  disconnect ( m_part, SIGNAL(destroyed()), this, SLOT(slotDestroyed()) );
}

void KateConsole::loadConsoleIfNeeded()
{
  if (m_part) return;

  if (!window() || !parentWidget()) return;
  if (!window() || !isVisibleTo(window())) return;

  KLibFactory *factory = KLibLoader::self()->factory("libkonsolepart");

  if (!factory) return;

  m_part = static_cast<KParts::ReadOnlyPart *>(factory->create(this,"libkonsolepart", "KParts::ReadOnlyPart"));

  if (!m_part) return;

  KGlobal::locale()->insertCatalogue("konsole");

  m_part->widget()->show();

  connect ( m_part, SIGNAL(destroyed()), this, SLOT(slotDestroyed()) );

  if (m_mw->activeView())
    if (m_mw->activeView()->document()->url().isValid())
      cd(KURL( m_mw->activeView()->document()->url().path() ));
}

void KateConsole::slotDestroyed ()
{
  m_part = 0;

  // hide the dockwidget
  if (parentWidget())
  {
    m_mw->hideToolView (m_toolView);
    m_mw->centralWidget()->setFocus ();
  }
}

void KateConsole::showEvent(QShowEvent *)
{
  if (m_part) return;

  loadConsoleIfNeeded();
}

void KateConsole::cd (const KURL &url)
{
  loadConsoleIfNeeded();

  if (!m_part) return;

  m_part->openURL (url);
}

void KateConsole::sendInput( const QString& text )
{
  loadConsoleIfNeeded();

  if (!m_part) return;

  if (!m_part->inherits ("TerminalInterface"))
    return;

  TerminalInterface *t = (TerminalInterface*) ( m_part );

  if (!t) return;

  t->sendInput (text);
}

void KateConsole::slotPipeToConsole ()
{
  if (KMessageBox::warningContinueCancel
      (m_mw->window()
       , i18n ("Do you really want to pipe the text to the console? This will execute any contained commands with your user rights.")
       , i18n ("Pipe to Console?")
       , i18n("Pipe to Console"), "Pipe To Console Warning") != KMessageBox::Continue)
    return;

  KTextEditor::View *v = m_mw->activeView();

  if (!v)
    return;
#warning "KDE4 it's not into kdelibs-snapshot for the moment";    
#if 0
  if (v->hasSelection())
    sendInput (v->selectionText());
  else
    sendInput (v->document()->text());
#endif
}
