/* This file is part of the KDE project
   Copyright (C) xxxx KFile Authors
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

#include "katefileselector.h"

#include <stdio.h>
#include <stdlib.h>

#include <qtextstream.h>
//Added by qt3to4:
#include <Q3CString>

#include <kbookmarkimporter.h>
#include <kpopupmenu.h>
#include <ksavefile.h>
#include <kstandarddirs.h>
#include <kdiroperator.h>
#include <kaction.h>

#include "katefileselector.h"

#include "kbookmarkhandler.h"
#include "kbookmarkhandler.moc"

Kate::Private::Plugin::KBookmarkHandler::KBookmarkHandler( KateFileSelector *parent, KPopupMenu* kpopupmenu )
    : QObject( parent, "KBookmarkHandler" ),
      KBookmarkOwner(),
      mParent( parent ),
      m_menu( kpopupmenu ),
      m_importStream( 0L )
{
    if (!m_menu)
      m_menu = new KPopupMenu( parent, "bookmark menu" );

    QString file = locate( "data", "kate/fsbookmarks.xml" );
    if ( file.isEmpty() )
        file = locateLocal( "data", "kate/fsbookmarks.xml" );

    KBookmarkManager *manager = KBookmarkManager::managerForFile( file, false);
    manager->setUpdate( true );
    manager->setShowNSBookmarks( false );

    m_bookmarkMenu = new KBookmarkMenu( manager, this, m_menu, 0, true );
}

Kate::Private::Plugin::KBookmarkHandler::~KBookmarkHandler()
{
    //     delete m_bookmarkMenu; ###
}

QString Kate::Private::Plugin::KBookmarkHandler::currentURL() const
{
    return mParent->dirOperator()->url().url();
}


void Kate::Private::Plugin::KBookmarkHandler::slotNewBookmark( const QString& /*text*/,
                                            const Q3CString& url,
                                            const QString& additionalInfo )
{
    *m_importStream << "<bookmark icon=\"" << KMimeType::iconForURL( KURL( url ) );
    *m_importStream << "\" href=\"" << QString::fromUtf8(url) << "\">\n";
    *m_importStream << "<title>" << (additionalInfo.isEmpty() ? QString::fromUtf8(url) : additionalInfo) << "</title>\n</bookmark>\n";
}

void Kate::Private::Plugin::KBookmarkHandler::slotNewFolder( const QString& text, bool /*open*/,
                                          const QString& /*additionalInfo*/ )
{
    *m_importStream << "<folder icon=\"bookmark_folder\">\n<title=\"";
    *m_importStream << text << "\">\n";
}

void Kate::Private::Plugin::KBookmarkHandler::newSeparator()
{
    *m_importStream << "<separator/>\n";
}

void Kate::Private::Plugin::KBookmarkHandler::endFolder()
{
    *m_importStream << "</folder>\n";
}

void Kate::Private::Plugin::KBookmarkHandler::virtual_hook( int id, void* data )
{ KBookmarkOwner::virtual_hook( id, data ); }

