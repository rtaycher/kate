/***************************************************************************
                          kantpartdocument.cpp  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : crossfire@babylon2k.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kantpartdocument.h"
#include "kantpartview.h"

#include <klocale.h>
#include <kcharsets.h>
#include <kdebug.h>

#include <kglobalsettings.h>
#include <kaction.h>
#include <kstdaction.h>

KantPartDocument::KantPartDocument (bool bSingleViewMode, bool bBrowserView,
                                                       QWidget *parentWidget, const char *widgetName,
                                                       QObject *parent, const char *name) :  KantDocument (0L, 0L)
{
  m_bSingleViewMode = bSingleViewMode;

  if ( m_bSingleViewMode )
  {
    KTextEditor::View *view = createView( parentWidget, widgetName );
    view->show();
    setWidget( view );

    if ( bBrowserView )
    {
      // We are embedded in konqueror, let's provide an XML file and actions.
      (void)new KantPartBrowserExtension( this );
      setXMLFile( "kantpartbrowserui.rc" );

      KStdAction::selectAll( view, SLOT( selectAll() ), actionCollection(), "select_all" );
      (void)new KAction( i18n( "Unselect all" ), 0, view, SLOT( deselectAll() ), actionCollection(), "unselect_all" );
      KStdAction::find( view, SLOT( find() ), actionCollection(), "find" );
      KStdAction::findNext( view, SLOT( findAgain() ), actionCollection(), "find_again" );
      KStdAction::gotoLine( view, SLOT( gotoLine() ), actionCollection(), "goto_line" );
    }
  }
}

KantPartDocument::~KantPartDocument ()
{
  if ( !m_bSingleViewMode )
  {
    m_views.setAutoDelete( true );
    m_views.clear();
    m_views.setAutoDelete( false );
  }
}

KTextEditor::View *KantPartDocument::createView( QWidget *parent, const char *name )
{
  return new KantPartView( this, parent, name );
}

KantPartBrowserExtension::KantPartBrowserExtension( KantPartDocument *doc )
: KParts::BrowserExtension( doc, "kantpartbrowserextension" )
{
  m_doc = doc;
  connect( m_doc, SIGNAL( selectionChanged() ), this, SLOT( slotSelectionChanged() ) );
}

void KantPartBrowserExtension::copy()
{
  m_doc->copy( 0 );
}

void KantPartBrowserExtension::slotSelectionChanged()
{
  emit enableAction( "copy", m_doc->hasMarkedText() );
}
