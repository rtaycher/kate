/* This file is part of the KDE libraries
   Copyright (C) 2002 Joseph Wenninger <jowenn@jowenn.at> and Daniel Naber <daniel.naber@t-online.de>
   
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

// $Id: $

#include "kate_kdatatool.h"
#include "kate_kdatatool.moc"
#include <kgenericfactory.h>
#include <kaction.h>
#include <ktexteditor/view.h>
#include <kdebug.h>
#include <kdatatool.h>
#include <ktexteditor/document.h>
#include <ktexteditor/selectioninterface.h>
#include <kpopupmenu.h>
#include <ktexteditor/viewcursorinterface.h>
#include <ktexteditor/editinterface.h>

K_EXPORT_COMPONENT_FACTORY( ktexteditor_kdatatool, KGenericFactory<KTextEditor::KDataToolPlugin>( "ktexteditor_kdatatool" ) );

namespace KTextEditor {

KDataToolPlugin::KDataToolPlugin( QObject *parent, const char* name, const QStringList& )
	: KTextEditor::Plugin ( (KTextEditor::Document*) parent, name )
{
}


KDataToolPlugin::~KDataToolPlugin ()
{
}

void KDataToolPlugin::addView(KTextEditor::View *view)
{
	KDataToolPluginView *nview = new KDataToolPluginView (view);
	nview->setView (view);
	//m_views.append (nview);
}


KDataToolPluginView::KDataToolPluginView( KTextEditor::View *view )
	:m_menu(0)
{

	view->insertChildClient (this);
	setInstance( KGenericFactory<KDataToolPlugin>::instance() );

	m_menu = new KActionMenu(i18n("Datatools"), actionCollection(), "popup_dataTool");
	connect(m_menu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(aboutToShow()));
	setXMLFile("ktexteditor_kdatatoolui.rc");

	m_view = view;
}

KDataToolPluginView::~KDataToolPluginView()
{
	delete m_menu;
}

void KDataToolPluginView::aboutToShow()
{
	kdDebug()<<"KTextEditor::KDataToolPluginView::aboutToShow"<<endl;
	QString word;
	m_singleWord = false;
	m_wordUnderCursor = QString::null;

	// unplug old actions, if any:
	KAction *ac;
	for ( ac = m_actionList.first(); ac; ac = m_actionList.next() ) {
		m_menu->remove(ac);
	}

	if ( selectionInterface(m_view->document())->hasSelection() )
	{
		word = selectionInterface(m_view->document())->selection();
		if ( word.find(' ') == -1 && word.find('\t') == -1 && word.find('\n') == -1 )
			m_singleWord = true;
		else
			m_singleWord = false;
	} else {
		// No selection -> use word under cursor
		KTextEditor::EditInterface *ei;
		KTextEditor::ViewCursorInterface *ci;
		KTextEditor::View *v = (KTextEditor::View*)m_view; 
		ei = KTextEditor::editInterface(v->document());
		ci = KTextEditor::viewCursorInterface(v);
		uint line, col;
		ci->cursorPositionReal(&line, &col);
		QString tmp_line = ei->textLine(line);
		m_wordUnderCursor = "";
		// find begin of word:
		for(uint i = col; i > 0; i--) {
			QChar ch = tmp_line.at(i);
			if( ch.isSpace() )
			{
				m_singleWord_start = i+1;
				break;
			}
			m_wordUnderCursor = ch + m_wordUnderCursor;
		}
		// find end of word:
		for(uint i = col+1; i < tmp_line.length(); i++) {
			QChar ch = tmp_line.at(i);
			if( ch.isSpace() )
			{
				m_singleWord_end = i;
				break;
			}
			m_wordUnderCursor += ch;
		}
		if( ! m_wordUnderCursor.isEmpty() )
		{
			m_singleWord = true;
			m_singleWord_line = line;
		} else {
			return;
		}
	}

	KInstance *inst=instance();

	QValueList<KDataToolInfo> tools;
	tools += KDataToolInfo::query( "QString", "text/plain", inst );
	if( m_singleWord )
		tools += KDataToolInfo::query( "QString", "application/x-singleword", inst );

	m_actionList = KDataToolAction::dataToolActionList( tools, this,
		SLOT( slotToolActivated( const KDataToolInfo &, const QString & ) ) );

	for ( ac = m_actionList.first(); ac; ac = m_actionList.next() ) {
		m_menu->insert(ac);
	}
	QString note(i18n("(not available)"));
	/* fixme: don't show an empty submenu
	if( m_actionList.isEmpty() ) {
	KAction *empty = new KAction(i18n("(not available)"), QString::null, 0, this, SLOT(slotNotAvailable()));
	m_menu->insert(empty);
	} */
}

void KDataToolPluginView::slotToolActivated( const KDataToolInfo &info, const QString &command )
{

	KDataTool* tool = info.createTool( );
	if ( !tool )
	{
		kdWarning() << "Could not create Tool !" << endl;
		return;
	}

	QString text;
	if ( selectionInterface(m_view->document())->hasSelection() )
		text = selectionInterface(m_view->document())->selection();
	else
		text = m_wordUnderCursor;

	QString mimetype = "text/plain";
	QString datatype = "QString";

	// If unsupported (and if we have a single word indeed), try application/x-singleword
	if ( !info.mimeTypes().contains( mimetype ) && m_singleWord )
		mimetype = "application/x-singleword";

	kdDebug() << "Running tool with datatype=" << datatype << " mimetype=" << mimetype << endl;

	QString origText = text;

	if ( tool->run( command, &text, datatype, mimetype) )
	{
		kdDebug() << "Tool ran. Text is now " << text << endl;
		if ( origText != text )
		{
			uint line, col;
			viewCursorInterface(m_view)->cursorPositionReal(&line, &col);
			if ( ! selectionInterface(m_view->document())->hasSelection() )
			{
				KTextEditor::SelectionInterface *si;
				si = KTextEditor::selectionInterface(m_view->document());
				si->setSelection(m_singleWord_line, m_singleWord_start, m_singleWord_line, m_singleWord_end);
			}
		
			// replace selection with 'text'
			selectionInterface(m_view->document())->removeSelectedText();
			viewCursorInterface(m_view)->cursorPositionReal(&line, &col);
			editInterface(m_view->document())->insertText(line, col, text);
			 // fixme: place cursor at the end:
			 /* No idea yet (Joseph Wenninger)
			 for ( uint i = 0; i < text.length(); i++ ) {
				viewCursorInterface(m_view)->cursorRight();
			 } */
		}
	}

	delete tool;
}


};
