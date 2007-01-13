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

#ifndef __KATE_FINDINFILES_H__
#define __KATE_FINDINFILES_H__

#include <kate/interfaces/plugin.h>
#include <kate/interfaces/mainwindow.h>
#include <kurl.h>
#include <kxmlguiclient.h>

#include <kvbox.h>
#include <QLinkedList>

#include <kategrepdialog.h>

class QShowEvent;

namespace KParts {
  class ReadOnlyPart;
}

namespace KateMDI {
  class ToolView;
}

namespace Kate {
namespace Private {
namespace Plugin {
class KateFindInFilesView;

class KateFindInFilesPlugin:public Kate::Plugin,public Kate::PluginViewInterface {
    Q_OBJECT
    Q_INTERFACES(Kate::PluginViewInterface)
  public:
    KateFindInFilesPlugin( QObject* parent = 0, const QStringList& = QStringList() );
    virtual ~KateFindInFilesPlugin(){}
    void addView (Kate::MainWindow *win);
    void removeView (Kate::MainWindow *win);

    void storeViewConfig(KConfig*,Kate::MainWindow *,const QString&) {}
    void loadViewConfig(KConfig*,Kate::MainWindow *,const QString&) {}
    void storeGeneralConfig(KConfig*,const QString&) {}
    void loadGeneralConfig(KConfig*,const QString&) {}
  private:
    QLinkedList<KateFindInFilesView*> m_views;
};

/**
 * KateFindInFilesView
 * This class is used for the internal terminal emulator
 * It uses internally the konsole part, thx to konsole devs :)
 */
class KateFindInFilesView : QObject
{
  Q_OBJECT

  public:
    /**
     * construct us
     * @param mw main window
     * @param parent toolview
     */
    KateFindInFilesView (Kate::MainWindow *mw);

    /**
     * destruct us
     */
    ~KateFindInFilesView ();

    Kate::MainWindow *mainWindow() {return m_mw;}

  private:
    /**
     * main window of this console
     */
    Kate::MainWindow *m_mw;

    /**
     * toolview for this console
     */
    QWidget *m_toolView;

    /**
     * Grepdialog
     */
    KateGrepDialog *m_grepDialog;
};

}
}
}
#endif
