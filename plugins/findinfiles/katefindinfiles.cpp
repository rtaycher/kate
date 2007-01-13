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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katefindinfiles.h"
#include "katefindinfiles.moc"

#include <kicon.h>
#include <kiconloader.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <kde_terminal_interface.h>

#include <kparts/part.h>
#include <kaction.h>
#include <kactioncollection.h>

#include <kurl.h>
#include <klibloader.h>
#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kapplication.h>
//Added by qt3to4:
#include <QShowEvent>

#include <kgenericfactory.h>
#include <kauthorized.h>

K_EXPORT_COMPONENT_FACTORY( KateFindInFilesPlugin, KGenericFactory<Kate::Private::Plugin::KateFindInFilesPlugin>( "KateFindInFilesPlugin" ) )

using namespace Kate::Private::Plugin;

KateFindInFilesPlugin::KateFindInFilesPlugin( QObject* parent, const QStringList& ):
  Kate::Plugin ( (Kate::Application*)parent ) {
  if (!KAuthorized::authorizeKAction("shell_access")) {
    KMessageBox::sorry(0, i18n ("You do not have enough karma to access a shell or terminal emulation"));
  }
}

void KateFindInFilesPlugin::addView(Kate::MainWindow *win) {
  kDebug()<<"KateFindInFilesPlugin::createView"<<endl;
  // ONLY ALLOW SHELL ACCESS IF ALLOWED ;)
  if (KAuthorized::authorizeKAction("shell_access")) {
    kDebug()<<"After auth check"<<endl;
    QWidget *toolview=win->createToolView ("kate_private_plugin_KateFindInFilesPlugin", MainWindow::Bottom, SmallIcon("konsole"), i18n("Terminal"));
    m_views.append(new KateFindInFilesView(win,toolview));
  }
}

void KateFindInFilesPlugin::removeView(Kate::MainWindow *win) {
  for(QLinkedList<KateFindInFilesView*>::iterator it=m_views.begin();it!=m_views.end();++it) {
    if ((*it)->mainWindow()==win) {
      delete *it;
      m_views.erase(it);
      break;
    }
  }
}

KateFindInFilesView::KateFindInFilesView (Kate::MainWindow *mw, QWidget *parent)
 : m_mw (mw)
 , m_toolView (parent)
{
}

KateFindInFilesView::~KateFindInFilesView ()
{
  delete m_toolView;
}
