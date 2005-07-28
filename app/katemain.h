/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_MAIN_H__
#define __KATE_MAIN_H__

#include <config.h>
//Added by qt3to4:
#include <QPixmap>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QEvent>

#define KATE_VERSION "2.5"

class QComboBox;
class QDateTime;
class QEvent;
class QFileInfo;
class QGridLayout;
class QLabel;
class Q3ListBox;
class QObject;
class QPixmap;
class QVBoxLayout;
class QString;
class Q3WidgetStack;

class KAction;
class KActionMenu;
class KConfig;
class KDirOperator;
class KEditToolbar;
class KFileViewItem;
class KHistoryCombo;
class KLineEdit;
class KListBox;
class KProcess;
class KPushButton;
class KRecentFilesAction;
class KSelectAction;
class KStatusBar;
class KToggleAction;
class KURL;
class KURLComboBox;

class KateApp;
class KateConfigDlg;
class KateConsole;
class KateDocManager;
class KateFileList;
class KateFileSelector;
class KateMainWindow;
class KatePluginIface;
class KatePluginManager;
class KateSidebar;
class KateViewManager;
class KateViewSpace;

#endif
