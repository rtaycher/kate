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

#include "../interfaces/view.h"
#include "../interfaces/document.h"
#include "kateviewdialog.h"
#include "katedocument.h" //FIXME remove

QStringList KateSearch::s_searchList  = QStringList();
QStringList KateSearch::s_replaceList = QStringList();

KateSearch::KateSearch( Kate::View* view )
	: QObject( view, "kate search" )
	, m_view( view )
	, m_doc( view->getDoc() )
	, _searchFlags( 0 )
	, replacePrompt( new ReplacePrompt( view ) )
{
	connect(replacePrompt,SIGNAL(clicked()),this,SLOT(replaceSlot()));
// TODO: Configuration
//  _searchFlags = config->readNumEntry("SearchFlags", SConfig::sfPrompt);
//   config->writeEntry("SearchFlags",_searchFlags);
}

KateSearch::~KateSearch()
{
}

void KateSearch::createActions( KActionCollection* ac )
{
	KStdAction::find(this, SLOT(find()), ac);
	KStdAction::findNext(this, SLOT(slotFindNext()), ac);
	KStdAction::findPrev(this, SLOT(slotFindPrev()), ac, "edit_find_prev");
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

void KateSearch::find()
{
	if (!doc()->hasSelection())
		_searchFlags &= ~SConfig::sfSelected;
	
	SearchDialog* searchDialog = new SearchDialog(
	    view(), s_searchList, s_replaceList,
	    _searchFlags & ~SConfig::sfReplace );
	
	searchDialog->setSearchText( getSearchText() );
	
	if( searchDialog->exec() == QDialog::Accepted ) {
		addToSearchList( searchDialog->getSearchFor() );
		_searchFlags = searchDialog->getFlags() | ( _searchFlags & SConfig::sfPrompt );
		initSearch( _searchFlags);
		findAgain();
	}
	delete searchDialog;
}

void KateSearch::replace()
{
	if (!doc()->isReadWrite()) return;
	
	if (!doc()->hasSelection())
		_searchFlags &= ~SConfig::sfSelected;
	
	SearchDialog* searchDialog = new SearchDialog(
	    view(), s_searchList, s_replaceList,
	    _searchFlags | SConfig::sfReplace );
	
	searchDialog->setSearchText( getSearchText() );
	
	if( searchDialog->exec() == QDialog::Accepted ) {
		addToSearchList( searchDialog->getSearchFor() );
		addToReplaceList( searchDialog->getReplaceWith() );
		_searchFlags = searchDialog->getFlags();
		initSearch( _searchFlags );
		replaceAgain();
	}
	delete searchDialog;
}

KateTextCursor KateSearch::getCursor()
{
	KateTextCursor c;
	c.line = view()->cursorLine();
	c.col = view()->cursorColumnReal();
	return c;
}

void KateSearch::initSearch( int flags )
{
	s.flags = flags;
	s.setPattern( s_searchList.first() );
	
	if( !(s.flags & SConfig::sfFromBeginning) ) {
		// If we are continuing a backward search, make sure we do not get stuck
		// at an existing match.
		s.cursor = getCursor();
		QString txt = view()->currentTextLine();
		QString searchFor = s_searchList.first();
		int pos = s.cursor.col-searchFor.length()-1;
		if ( pos < 0 )
			pos = 0;
		pos = txt.find( searchFor, pos, s.flags & SConfig::sfCaseSensitive );
		if ( s.flags & SConfig::sfBackward ) {
			if ( pos <= s.cursor.col )
				s.cursor.col = pos-1;
		} else {
			if ( pos == s.cursor.col )
				s.cursor.col++;
		}
	} else {
		if (!(s.flags & SConfig::sfBackward)) {
			s.cursor.col = 0;
			s.cursor.line = 0;
		} else {
			s.cursor.col = -1;
			s.cursor.line = doc()->numLines();
		}
		s.flags |= SConfig::sfFinished;
	}
	if (!(s.flags & SConfig::sfBackward)) {
		if (!(s.cursor.col || s.cursor.line))
			s.flags |= SConfig::sfFinished;
	}
}

void KateSearch::findAgain (bool back)
{
  bool b= (_searchFlags & SConfig::sfBackward) > 0;
  initSearch((_searchFlags & ((b==back)?~SConfig::sfBackward:~0) & ~SConfig::sfFromBeginning)
                | SConfig::sfPrompt | SConfig::sfAgain | ((b!=back)?SConfig::sfBackward : 0) );
  if (s.flags & SConfig::sfReplace)
    replaceAgain();
  else
    findAgain();
}

void KateSearch::findAgain()
{
	QString searchFor = s_searchList.first();
	
	if( searchFor.isEmpty() ) {
		find();
		return;
	}
	
	bool _continue = false;
	do {
		if ( ((KateDocument*)doc())->doSearch(s,searchFor)) { // FIXME
			KateTextCursor cursor = s.cursor;
			if (!(s.flags & SConfig::sfBackward))
				s.cursor.col += s.matchedLength;
			exposeFound( cursor, s.matchedLength );
			return;
		} else {
			if( !(s.flags & SConfig::sfFinished) ) {
				_continue = askContinue( !(s.flags & SConfig::sfBackward), false, 0 );
				continueSearch();
			} else {
				// wrapped
				KMessageBox::sorry( view(),
				    i18n("Search string '%1' not found!")
				         .arg( KStringHandler::csqueeze( searchFor ) ),
				    i18n("Find"));
			}
		}
	} while( _continue );
}

void KateSearch::replaceAgain()
{
	if (!doc()->isReadWrite())
		return;
	
	replaces = 0;
	if (s.flags & SConfig::sfPrompt) {
		doReplaceAction(-1);
	} else {
		doReplaceAction(srAll);
	}
}

void KateSearch::doReplaceAction(int result, bool found) {
  int rlen;
  bool started;

  QString searchFor = s_searchList.first();
  QString replaceWith = s_replaceList.first();
  rlen = replaceWith.length();

  switch (result) {
    case srYes: //yes
      doc()->removeText (s.cursor.line, s.cursor.col, s.cursor.line, s.cursor.col + s.matchedLength);
      doc()->insertText (s.cursor.line, s.cursor.col, replaceWith);
      replaces++;

      if (!(s.flags & SConfig::sfBackward))
            s.cursor.col += rlen;
          else
          {
            if (s.cursor.col > 0)
              s.cursor.col--;
            else
            {
              s.cursor.line--;

              if (s.cursor.line >= 0)
              {
                s.cursor.col = doc()->lineLength(s.cursor.line);
              }
            }
          }

      break;
    case srNo: //no
      if (!(s.flags & SConfig::sfBackward))
            s.cursor.col += s.matchedLength;
          else
          {
            if (s.cursor.col > 0)
              s.cursor.col--;
            else
            {
              s.cursor.line--;

              if (s.cursor.line >= 0)
              {
                s.cursor.col = doc()->lineLength(s.cursor.line);
              }
            }
          }
     
      break;
    case srAll: //replace all
      do {
        started = false;
	// FIXME
        while (found || ((KateDocument*)doc())->doSearch(s,searchFor)) {
          if (!started) {
            found = false;
            started = true;
          }
          doc()->removeText (s.cursor.line, s.cursor.col, s.cursor.line, s.cursor.col + s.matchedLength);
          doc()->insertText (s.cursor.line, s.cursor.col, replaceWith);
          replaces++;

          if (!(s.flags & SConfig::sfBackward))
            s.cursor.col += rlen;
          else
          {
            if (s.cursor.col > 0)
              s.cursor.col--;
            else
            {
              s.cursor.line--;

              if (s.cursor.line >= 0)
              {
                s.cursor.col = doc()->lineLength(s.cursor.line);
              }
            }
          }

        }
      } while (askReplaceEnd());
      return;
    case srCancel: //cancel
      return;
    default:
      break;
  }

	do {
		if ( ((KateDocument*)doc())->doSearch(s,searchFor)) { // FIXME
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
	if( s.flags & SConfig::sfFinished ) {
		KMessageBox::information( view(),
		    i18n("%1 replacement(s) made").arg(replaces),
		    i18n("Replace") );
		return false;
	}
		
	bool _continue = askContinue( !(s.flags & SConfig::sfBackward), true, replaces );
	replaces = 0;
	continueSearch();
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

void KateSearch::continueSearch()
{
  if (!(s.flags & SConfig::sfBackward)) {
    s.cursor.col = 0;
    s.cursor.line = 0;
  } else {
    s.cursor.col = -1;
    s.cursor.line = doc()->numLines();
  }
  s.flags |= SConfig::sfFinished;
  s.flags &= ~SConfig::sfAgain;
}

void KateSearch::replaceSlot() {
  doReplaceAction(replacePrompt->result(),true);
}

void KateSearch::exposeFound( KateTextCursor &cursor, int slen )
{
  view()->setCursorPositionReal( cursor.line, cursor.col + slen );
  doc()->setSelection( cursor.line, cursor.col, cursor.line, cursor.col + slen );
}

// vim: noet ts=2
