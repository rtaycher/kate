/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>  
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#include "katesearch.h"
#include "katesearch.moc"

#include <klocale.h>
#include <kstdaction.h>
#include <kmessagebox.h>
#include <kstringhandler.h>
#include <kdebug.h>

#include "../interfaces/view.h"
#include "../interfaces/document.h"
#include "kateviewdialog.h"

QStringList KateSearch::s_searchList  = QStringList();
QStringList KateSearch::s_replaceList = QStringList();

KateSearch::KateSearch( Kate::View* view )
	: QObject( view, "kate search" )
	, m_view( view )
	, m_doc( view->getDoc() )
	, m_searchFlags()
	, replacePrompt( new ReplacePrompt( view ) )
{
	connect(replacePrompt,SIGNAL(clicked()),this,SLOT(replaceSlot()));
// TODO: Configuration
//  m_searchFlags = config->readNumEntry("SearchFlags", SConfig::sfPrompt);
//   config->writeEntry("SearchFlags",m_searchFlags);
}

KateSearch::~KateSearch()
{
}

void KateSearch::createActions( KActionCollection* ac )
{
	KStdAction::find( this, SLOT(find()), ac );
	KStdAction::findNext( this, SLOT(slotFindNext()), ac );
	KStdAction::findPrev( this, SLOT(slotFindPrev()), ac, "edit_find_prev" );
}

void KateSearch::addToList( QStringList& list, const QString& s )
{
	if( list.count() > 0 ) {
		QStringList::Iterator it = list.find( s );
		if( *it != 0L )
			list.remove( it );
		if( list.count() >= 16 )
			list.remove( list.fromLast() );
	}
	list.prepend( s );
}

void KateSearch::find()
{
	m_searchFlags.selected &= doc()->hasSelection();
	m_searchFlags.replace = false;
	
	SearchDialog* searchDialog = new SearchDialog(
	    view(), s_searchList, s_replaceList,
	    m_searchFlags );
	
	searchDialog->setSearchText( getSearchText() );
	
	if( searchDialog->exec() == QDialog::Accepted ) {
		addToSearchList( searchDialog->getSearchFor() );
		bool prompt = m_searchFlags.prompt;
		m_searchFlags = searchDialog->getFlags();
		m_searchFlags.prompt = prompt;
		search( m_searchFlags );
	}
	delete searchDialog;
}

void KateSearch::replace()
{
	if (!doc()->isReadWrite()) return;
	
	m_searchFlags.selected &= doc()->hasSelection();
	m_searchFlags.replace = true;
	
	SearchDialog* searchDialog = new SearchDialog(
	    view(), s_searchList, s_replaceList,
	    m_searchFlags );
	
	searchDialog->setSearchText( getSearchText() );
	
	if( searchDialog->exec() == QDialog::Accepted ) {
		addToSearchList( searchDialog->getSearchFor() );
		addToReplaceList( searchDialog->getReplaceWith() );
		m_searchFlags = searchDialog->getFlags();
		search( m_searchFlags );
	}
	delete searchDialog;
}

void KateSearch::findAgain( bool back )
{
	SearchFlags flags = m_searchFlags;
	flags.backward = m_searchFlags.backward != back;
	flags.fromBeginning = false;
	flags.prompt = true;
	
	search( flags );
}

void KateSearch::search( SearchFlags flags )
{
	s.flags = flags;
	s.cursor = getCursor();
	
	if( s.flags.fromBeginning ) {
		if( !s.flags.backward ) {
			s.cursor.col = 0;
			s.cursor.line = 0;
		} else {
			s.cursor.line = doc()->numLines() - 1;
			s.cursor.col = doc()->lineLength( s.cursor.line );
		}
	} else if( s.flags.backward ) {
		// If we are continuing a backward search, make sure
		// we do not get stuck at an existing match.
		QString txt = view()->currentTextLine();
		QString searchFor = s_searchList.first();
		uint length = searchFor.length();
		int pos = s.cursor.col - length;
		kdDebug() << pos << ", " << length << ": " << txt.mid( pos, length ) << endl;
		if( searchFor.find( txt.mid( pos, length ), 0, s.flags.caseSensitive ) == 0 ) {
			if( pos > 0 ) {
				s.cursor.col = pos - 1;
			} else if ( pos == 0 && s.cursor.line > 0 ) {
				s.cursor.line--;
				s.cursor.col = doc()->lineLength( s.cursor.line );
			} else if ( pos == 0 && s.cursor.line == 0 ) {
				// TODO: FIXME
			}
		}
	}
	if((!s.flags.backward && 
	     s.cursor.col == 0 && 
	     s.cursor.line == 0 ) ||
	   ( s.flags.backward && 
	     s.cursor.col == doc()->lineLength( s.cursor.line ) && 
	     s.cursor.line == doc()->numLines()-1 ) ) {
		s.flags.finished = true;
	}
	
	if( s.flags.replace ) {
		replaceAgain();
	} else {
		findAgain();
	}
}

bool KateSearch::doSearch( const QString& text )
{
	uint line = s.cursor.line;
	uint col = s.cursor.col;
	bool backward = s.flags.backward;
	bool caseSensitive = s.flags.caseSensitive;
	bool regExp = s.flags.regExp;
	bool wholeWords = s.flags.wholeWords;
	uint foundLine, foundCol, matchLen;
	bool found = false;
	kdDebug() << "Searching at " << line << ", " << col << endl;
	if( regExp ) {
		QRegExp re( text, caseSensitive );
		found = doc()->searchText( line, col, re,
		                           &foundLine, &foundCol,
		                           &matchLen, backward );
	} else if ( wholeWords ) {
		QRegExp re( "\\b" + text + "\\b", caseSensitive );
		found = doc()->searchText( line, col, re,
		                           &foundLine, &foundCol,
		                           &matchLen, backward );
	} else {
		found = doc()->searchText( line, col, text,
		                           &foundLine, &foundCol,
		                           &matchLen, caseSensitive, backward );
	}
	if( !found ) return false;
	kdDebug() << "Found at " << foundLine << ", " << foundCol << endl;
	s.cursor.line = foundLine;
	s.cursor.col = foundCol;
	s.matchedLength = matchLen;
	return true;
}

void KateSearch::wrapSearch()
{
	if( !s.flags.backward ) {
		s.cursor.col = 0;
		s.cursor.line = 0;
	} else {
		s.cursor.line = doc()->numLines() - 1;
		s.cursor.col = doc()->lineLength( s.cursor.line );
	}
	s.flags.finished = true;
}

void KateSearch::findAgain()
{
	QString searchFor = s_searchList.first();
	
	if( searchFor.isEmpty() ) {
		find();
		return;
	}
	
	if ( doSearch( searchFor ) ) {
		exposeFound( s.cursor, s.matchedLength );
	} else if( !s.flags.finished ) {
		wrapSearch();
		if( askContinue( !s.flags.backward, false, 0 ) )
			findAgain();
	} else {
		KMessageBox::sorry( view(),
		    i18n("Search string '%1' not found!")
		         .arg( KStringHandler::csqueeze( searchFor ) ),
		    i18n("Find"));
	}
}

void KateSearch::replaceAgain()
{
	if (!doc()->isReadWrite())
		return;
	
	replaces = 0;
	if( s.flags.prompt ) {
		doReplaceAction(-1);
	} else {
		doReplaceAction(srAll);
	}
}

void KateSearch::doReplaceAction( int result, bool found )
{
	QString searchFor = s_searchList.first();
	QString replaceWith = s_replaceList.first();
	
	switch( result ) {
	case srYes:
		doc()->removeText( s.cursor.line, s.cursor.col,
		                   s.cursor.line, s.cursor.col + s.matchedLength );
		doc()->insertText( s.cursor.line, s.cursor.col, replaceWith );
		replaces++;
		if( !s.flags.backward ) {
			s.cursor.col += replaceWith.length();
		} else if( s.cursor.col > 0 ) {
			s.cursor.col--;
		} else {
			s.cursor.line--;
			if( s.cursor.line >= 0 ) {
				s.cursor.col = doc()->lineLength( s.cursor.line );
			}
		}
		break;
	
	case srNo:
		if( !s.flags.backward ) {
			s.cursor.col += s.matchedLength;
		} else if( s.cursor.col > 0 ) {
			s.cursor.col--;
		} else {
			s.cursor.line--;
			if( s.cursor.line >= 0 ) {
				s.cursor.col = doc()->lineLength(s.cursor.line);
			}
		}
		break;
		
	case srAll:
		do {
			bool started = false;
			while( found || doSearch( searchFor ) ) {
				if (!started) {
					found = false;
					started = true;
				}
				doc()->removeText( s.cursor.line, s.cursor.col,
				                   s.cursor.line, s.cursor.col + s.matchedLength );
				doc()->insertText( s.cursor.line, s.cursor.col, replaceWith );
				replaces++;
				if( !s.flags.backward ) {
					s.cursor.col += replaceWith.length();
				} else if (s.cursor.col > 0) {
					s.cursor.col--;
				} else {
					s.cursor.line--;
					if (s.cursor.line >= 0) {
						s.cursor.col = doc()->lineLength(s.cursor.line);
					}
				}
			}
		} while (askReplaceEnd());
		return;
	case srCancel:
		return;
	default:
		break;
	}
	
	do {
		if ( doSearch( searchFor ) ) {
			exposeFound( s.cursor, s.matchedLength );
			replacePrompt->show();
			return;
		}
		//nothing found: repeat until user cancels "repeat from beginning" dialog
	} while( askReplaceEnd() );
	replacePrompt->hide();
}

bool KateSearch::askReplaceEnd()
{
	if( s.flags.finished ) {
		KMessageBox::information( view(),
		    i18n("%1 replacement(s) made").arg(replaces),
		    i18n("Replace") );
		return false;
	}
	
	bool _continue = askContinue( !s.flags.backward, true, replaces );
	replaces = 0;
	wrapSearch();
	return _continue;
}

bool KateSearch::askContinue( bool forward, bool replace, int replacements )
{
	QString made =
	   i18n( "%n replacement made",
	         "%n replacements made",
	         replacements );
	QString reached = forward ?
	   i18n( "End of document reached." ) :
	   i18n( "Beginning of document reached." );
	QString question = forward ?
	   i18n( "Continue from the beginning?" ) :
	   i18n( "Continue from the end?" );
	QString text = replace ?
	   made + "\n" + reached + "\n" + question :
	   reached + "\n" + question;
	return KMessageBox::Yes == KMessageBox::questionYesNo(
	   view(), text, replace ? i18n("Replace") : i18n("Find"),
	   i18n("Continue"), i18n("Stop") );
}

void KateSearch::replaceSlot() {
	doReplaceAction(replacePrompt->result(),true);
}

QString KateSearch::getSearchText()
{
	// If the user has marked some text we use that otherwise
	// use the word under the cursor.
	QString str;
	
	if( doc()->hasSelection() )
		str = doc()->selection();
	else
		str = view()->currentWord();
	
	str.replace( QRegExp("^\\n"), "" );
	str.replace( QRegExp("\\n.*"), "" );
	
	return str;
}

KateTextCursor KateSearch::getCursor()
{
	KateTextCursor c;
	c.line = view()->cursorLine();
	c.col = view()->cursorColumnReal();
	return c;
}

void KateSearch::exposeFound( KateTextCursor &cursor, int slen )
{
	view()->setCursorPositionReal( cursor.line, cursor.col + slen );
	doc()->setSelection( cursor.line, cursor.col, cursor.line, cursor.col + slen );
}

// vim: noet ts=2
