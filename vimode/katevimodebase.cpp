/* This file is part of the KDE libraries
 * Copyright (C) 2008 - 2009 Erlend Hamberg <ehamberg@gmail.com>
 * Copyright (C) 2009 Paul Gideon Dann <pdgiddie@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "katevimodebase.h"
#include "katevirange.h"
#include "kateglobal.h"
#include "kateviglobal.h"
#include "katevivisualmode.h"
#include "katevinormalmode.h"
#include "katevireplacemode.h"
#include "kateviinputmodemanager.h"

#include <QString>
#include <QRegExp>
#include "katedocument.h"
#include "kateviewinternal.h"
#include "katevimodebar.h"

// TODO: the "previous word/WORD [end]" methods should be optimized. now they're being called in a
// loop and all calculations done up to finding a match are wasted when called with a count > 1
// because they will simply be called again from the last found position.
// They should take the count as a parameter and collect the positions in a QList, then return
// element (count - 1)

////////////////////////////////////////////////////////////////////////////////
// HELPER METHODS
////////////////////////////////////////////////////////////////////////////////

bool KateViModeBase::deleteRange( KateViRange &r, bool linewise, bool addToRegister)
{
  r.normalize();
  bool res = false;
  QString removedText = getRange( r, linewise );

  if ( linewise ) {
    doc()->editStart();
    for ( int i = 0; i < r.endLine-r.startLine+1; i++ ) {
      res = doc()->removeLine( r.startLine );
    }
    doc()->editEnd();
  } else {
      res = doc()->removeText( KTextEditor::Range( r.startLine, r.startColumn, r.endLine, r.endColumn) );
  }

  if ( addToRegister ) {
    if ( r.startLine == r.endLine ) {
      fillRegister( getChosenRegister( '-' ), removedText );
    } else {
      fillRegister( getChosenRegister( '0' ), removedText );
    }
  }

  return res;
}

const QString KateViModeBase::getRange( KateViRange &r, bool linewise) const
{
  r.normalize();
  QString s;

  if ( linewise ) {
    r.startColumn = 0;
    r.endColumn = getLine( r.endLine ).length();
  }

  if ( r.motionType == ViMotion::InclusiveMotion ) {
    r.endColumn++;
  }

  KTextEditor::Range range( r.startLine, r.startColumn, r.endLine, r.endColumn);

  if ( linewise ) {
    s = doc()->textLines( range ).join( QChar( '\n' ) );
    s.append( QChar( '\n' ) );
  } else {
      s = doc()->text( range );
  }

  return s;
}

const QString KateViModeBase::getLine( int lineNumber ) const
{
  QString line;

  if ( lineNumber == -1 ) {
    KTextEditor::Cursor cursor ( m_view->cursorPosition() );
    line = m_view->currentTextLine();
  } else {
    line = doc()->line( lineNumber );
  }

  return line;
}

const QChar KateViModeBase::getCharUnderCursor() const
{
  KTextEditor::Cursor c( m_view->cursorPosition() );

  QString line = getLine( c.line() );

  if ( line.length() == 0 && c.column() >= line.length() ) {
    return QChar::Null;
  }

  return line.at( c.column() );
}

KTextEditor::Cursor KateViModeBase::findNextWordStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  // the start of word pattern need to take m_extraWordCharacters into account if defined
  QString startOfWordPattern("\\b(\\w");
  if ( m_extraWordCharacters.length() > 0 ) {
    startOfWordPattern.append( QLatin1String( "|[" )+m_extraWordCharacters+']' );
  }
  startOfWordPattern.append( ')' );

  QRegExp startOfWord( startOfWordPattern );    // start of a word
  QRegExp nonSpaceAfterSpace( "\\s\\S" );       // non-space right after space
  QRegExp nonWordAfterWord( "\\b(?!\\s)\\W" );  // word-boundary followed by a non-word which is not a space

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
    int c1 = startOfWord.indexIn( line, c + 1 );
    int c2 = nonSpaceAfterSpace.indexIn( line, c );
    int c3 = nonWordAfterWord.indexIn( line, c + 1 );

    if ( c1 == -1 && c2 == -1 && c3 == -1 ) {
        if ( onlyCurrentLine ) {
            return KTextEditor::Cursor( l, c );
        } else if ( l >= doc()->lines()-1 ) {
            c = line.length()-1;
            return KTextEditor::Cursor( l, c );
        } else {
            c = 0;
            l++;

            line = getLine( l );

        if ( line.length() == 0 || !line.at( c ).isSpace() ) {
          found = true;
        }

        continue;
      }
    }

    c2++; // the second regexp will match one character *before* the character we want to go to

    if ( c1 <= 0 )
      c1 = line.length()-1;
    if ( c2 <= 0 )
      c2 = line.length()-1;
    if ( c3 <= 0 )
      c3 = line.length()-1;

    c = qMin( c1, qMin( c2, c3 ) );

    found = true;
  }

  return KTextEditor::Cursor( l, c );
}

KTextEditor::Cursor KateViModeBase::findNextWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  KTextEditor::Cursor cursor ( m_view->cursorPosition() );
  QString line = getLine();
  KateViRange r( cursor.line(), cursor.column(), ViMotion::ExclusiveMotion );

  int l = fromLine;
  int c = fromColumn;

  bool found = false;
  QRegExp startOfWORD("\\s\\S");

  while ( !found ) {
    c = startOfWORD.indexIn( line, c+1 );

    if ( c == -1 ) {
      if ( onlyCurrentLine ) {
          return KTextEditor::Cursor( l, c );
      } else if ( l >= doc()->lines()-1 ) {
        c = line.length()-1;
        break;
      } else {
        c = 0;
        l++;

        line = getLine( l );

        if ( line.length() == 0 || !line.at( c ).isSpace() ) {
          found = true;
        }

        continue;
      }
    } else {
      c++;
      found = true;
    }
  }

  return KTextEditor::Cursor( l, c );
}

KTextEditor::Cursor KateViModeBase::findPrevWordEnd( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  QString endOfWordPattern = "\\S\\s|\\S$|\\w\\W|\\S\\b|^$";

  if ( m_extraWordCharacters.length() > 0 ) {
   endOfWordPattern.append( "|["+m_extraWordCharacters+"][^" +m_extraWordCharacters+']' );
  }

  QRegExp endOfWord( endOfWordPattern );

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
      int c1 = endOfWord.lastIndexIn( line, c-1 );

      if ( c1 != -1 && c-1 != -1 ) {
          found = true;
          c = c1;
      } else {
          if ( onlyCurrentLine ) {
              return KTextEditor::Cursor( l, c );
          } else if ( l > 0 ) {
              line = getLine( --l );
              c = line.length();

              continue;
          } else {
              c = 0;
              return KTextEditor::Cursor( l, c );
          }
      }
  }

  return KTextEditor::Cursor( l, c );
}

KTextEditor::Cursor KateViModeBase::findPrevWORDEnd( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  QRegExp endOfWORDPattern( "\\S\\s|\\S$|^$" );

  QRegExp endOfWORD( endOfWORDPattern );

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
      int c1 = endOfWORD.lastIndexIn( line, c-1 );

      if ( c1 != -1 && c-1 != -1 ) {
          found = true;
          c = c1;
      } else {
          if ( onlyCurrentLine ) {
              return KTextEditor::Cursor( l, c );
          } else if ( l > 0 ) {
              line = getLine( --l );
              c = line.length();

              continue;
          } else {
              c = 0;
              return KTextEditor::Cursor( l, c );
          }
      }
  }

  return KTextEditor::Cursor( l, c );
}

KTextEditor::Cursor KateViModeBase::findPrevWordStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  // the start of word pattern need to take m_extraWordCharacters into account if defined
  QString startOfWordPattern("\\b(\\w");
  if ( m_extraWordCharacters.length() > 0 ) {
    startOfWordPattern.append( QLatin1String( "|[" )+m_extraWordCharacters+']' );
  }
  startOfWordPattern.append( ')' );

  QRegExp startOfWord( startOfWordPattern );    // start of a word
  QRegExp nonSpaceAfterSpace( "\\s\\S" );       // non-space right after space
  QRegExp nonWordAfterWord( "\\b(?!\\s)\\W" );  // word-boundary followed by a non-word which is not a space
  QRegExp startOfLine( "^\\S" );                  // non-space at start of line

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
    int c1 = startOfWord.lastIndexIn( line, -line.length()+c-1 );
    int c2 = nonSpaceAfterSpace.lastIndexIn( line, -line.length()+c-2 );
    int c3 = nonWordAfterWord.lastIndexIn( line, -line.length()+c-1 );
    int c4 = startOfLine.lastIndexIn( line, -line.length()+c-1 );

    if ( c1 == -1 && c2 == -1 && c3 == -1 && c4 == -1 ) {
      if ( onlyCurrentLine ) {
          return KTextEditor::Cursor( l, c );
      } else if ( l <= 0 ) {
        return KTextEditor::Cursor( 0, 0 );
      } else {
        line = getLine( --l );
        c = line.length()-1;

        if ( line.length() == 0 ) {
          c = 0;
          found = true;
        }

        continue;
      }
    }

    c2++; // the second regexp will match one character *before* the character we want to go to

    if ( c1 <= 0 )
      c1 = 0;
    if ( c2 <= 0 )
      c2 = 0;
    if ( c3 <= 0 )
      c3 = 0;
    if ( c4 <= 0 )
      c4 = 0;

    c = qMax( c1, qMax( c2, qMax( c3, c4 ) ) );

    found = true;
  }

  return KTextEditor::Cursor( l, c );
}

KTextEditor::Cursor KateViModeBase::findPrevWORDStart( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  QRegExp startOfWORD("\\s\\S");
  QRegExp startOfLineWORD("^\\S");

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
    int c1 = startOfWORD.lastIndexIn( line, -line.length()+c-2 );
    int c2 = startOfLineWORD.lastIndexIn( line, -line.length()+c-1 );

    if ( c1 == -1 && c2 == -1 ) {
      if ( onlyCurrentLine ) {
          return KTextEditor::Cursor( l, c );
      } else if ( l <= 0 ) {
        return KTextEditor::Cursor( 0, 0 );
      } else {
        line = getLine( --l );
        c = line.length()-1;

        if ( line.length() == 0 ) {
          c = 0;
          found = true;
        }

        continue;
      }
    }

    c1++; // the startOfWORD pattern matches one character before the word

    c = qMax( c1, c2 );

    if ( c <= 0 )
      c = 0;

    found = true;
  }

  return KTextEditor::Cursor( l, c );
}

KTextEditor::Cursor KateViModeBase::findWordEnd( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  QString endOfWordPattern = "\\S\\s|\\S$|\\w\\W|\\S\\b";

  if ( m_extraWordCharacters.length() > 0 ) {
   endOfWordPattern.append( "|["+m_extraWordCharacters+"][^" +m_extraWordCharacters+']' );
  }

  QRegExp endOfWORD( endOfWordPattern );

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
      int c1 = endOfWORD.indexIn( line, c+1 );

      if ( c1 != -1 ) {
          found = true;
          c = c1;
      } else {
          if ( onlyCurrentLine ) {
              return KTextEditor::Cursor( l, c );
          } else if ( l >= doc()->lines()-1 ) {
              c = line.length()-1;
              return KTextEditor::Cursor( l, c );
          } else {
              c = -1;
              line = getLine( ++l );

              continue;
          }
      }
  }

  return KTextEditor::Cursor( l, c );
}

KTextEditor::Cursor KateViModeBase::findWORDEnd( int fromLine, int fromColumn, bool onlyCurrentLine ) const
{
  QString line = getLine( fromLine );

  QRegExp endOfWORD( "\\S\\s|\\S$" );

  int l = fromLine;
  int c = fromColumn;

  bool found = false;

  while ( !found ) {
      int c1 = endOfWORD.indexIn( line, c+1 );

      if ( c1 != -1 ) {
          found = true;
          c = c1;
      } else {
          if ( onlyCurrentLine ) {
              return KTextEditor::Cursor( l, c );
          } else if ( l >= doc()->lines()-1 ) {
              c = line.length()-1;
              return KTextEditor::Cursor( l, c );
          } else {
              c = -1;
              line = getLine( ++l );

              continue;
          }
      }
  }

  return KTextEditor::Cursor( l, c );
}

// FIXME: i" won't work if the cursor is on one of the chars
KateViRange KateViModeBase::findSurrounding( const QChar &c1, const QChar &c2, bool inner ) const
{
  KTextEditor::Cursor cursor( m_view->cursorPosition() );
  QString line = getLine();

  int col1 = line.lastIndexOf( c1, cursor.column() );
  int col2 = line.indexOf( c2, cursor.column() );

  KateViRange r( cursor.line(), col1, cursor.line(), col2, ViMotion::InclusiveMotion );

  if ( col1 == -1 || col2 == -1 || col1 > col2 ) {
      r.valid = false;
  }

  if ( inner ) {
      r.startColumn++;
      r.endColumn--;
  }

  return r;
}

KateViRange KateViModeBase::findSurrounding( const QRegExp &c1, const QRegExp &c2, bool inner ) const
{
  KTextEditor::Cursor cursor( m_view->cursorPosition() );
  QString line = getLine();

  int col1 = line.lastIndexOf( c1, cursor.column() );
  int col2 = line.indexOf( c2, cursor.column() );

  KateViRange r( cursor.line(), col1, cursor.line(), col2, ViMotion::InclusiveMotion );

  if ( col1 == -1 || col2 == -1 || col1 > col2 ) {
      r.valid = false;
  }

  if ( inner ) {
      r.startColumn++;
      r.endColumn--;
  }

  return r;
}

int KateViModeBase::findLineStartingWitchChar( const QChar &c, unsigned int count, bool forward ) const
{
  int line = m_view->cursorPosition().line();
  int lines = doc()->lines();
  unsigned int hits = 0;

  if ( forward ) {
    line++;
  } else {
    line--;
  }

  while ( line < lines && line > 0 && hits < count ) {
    QString l = getLine( line );
    if ( l.length() > 0 && l.at( 0 ) == c ) {
      hits++;
    }
    if ( hits != count ) {
      if ( forward ) {
        line++;
      } else {
        line--;
      }
    }
  }

  if ( hits == getCount() ) {
    return line;
  }

  return -1;
}

void KateViModeBase::updateCursor( const KTextEditor::Cursor &c ) const
{
  m_viewInternal->updateCursor( c );
}

/**
 * @return the register given for the command. If no register was given, defaultReg is returned.
 */
QChar KateViModeBase::getChosenRegister( const QChar &defaultReg ) const
{
  QChar reg = ( m_register != QChar::Null ) ? m_register : defaultReg;

  return reg;
}

QString KateViModeBase::getRegisterContent( const QChar &reg ) const
{
  QString r = KateGlobal::self()->viInputModeGlobal()->getRegisterContent( reg );

  if ( r.isNull() ) {
    error( QString( "Nothing in register " ) + reg );
  }

  return r;
}

void KateViModeBase::fillRegister( const QChar &reg, const QString &text )
{
  KateGlobal::self()->viInputModeGlobal()->fillRegister( reg, text );
}

KateViRange KateViModeBase::goLineDown()
{
  return goLineUpDown( getCount() );
}

KateViRange KateViModeBase::goLineUp()
{
  return goLineUpDown( -getCount() );
}

/**
 * method for moving up or down one or more lines
 * note: the sticky column is always a virtual column
 */
KateViRange KateViModeBase::goLineUpDown( int lines )
{
  KTextEditor::Cursor c( m_view->cursorPosition() );
  KateViRange r( c.line(), c.column(), ViMotion::InclusiveMotion );
  int tabstop = doc()->config()->tabWidth();

  // if in an empty document, just return
  if ( lines == 0 ) {
    return r;
  }

  r.endLine += lines;

  // limit end line to be from line 0 through the last line
  if ( r.endLine < 0 ) {
    r.endLine = 0;
  } else if ( r.endLine > doc()->lines()-1 ) {
    r.endLine = doc()->lines()-1;
  }

  KateTextLine::Ptr startLine = doc()->plainKateTextLine( c.line() );
  KateTextLine::Ptr endLine = doc()->plainKateTextLine( r.endLine );

  int endLineLen = doc()->lineLength( r.endLine )-1;

  if ( endLineLen < 0 ) {
    endLineLen = 0;
  }

  int endLineLenVirt = endLine->toVirtualColumn(endLineLen, tabstop);
  int virtColumnStart = startLine->toVirtualColumn(c.column(), tabstop);

  // if sticky column isn't set, set end column and set sticky column to its virtual column
  if ( m_stickyColumn == -1 ) {
    r.endColumn = endLine->fromVirtualColumn( virtColumnStart, tabstop );
    m_stickyColumn = virtColumnStart;
  } else {
    // sticky is set - set end column to its value
    r.endColumn = endLine->fromVirtualColumn( m_stickyColumn, tabstop );
  }

  // make sure end column won't be after the last column of a line
  if ( r.endColumn > endLineLen ) {
    r.endColumn = endLineLen;
  }

  // if we move to a line shorter than the current column, go to its end
  if ( virtColumnStart > endLineLenVirt ) {
    r.endColumn = endLineLen;
  }

  return r;
}

bool KateViModeBase::startNormalMode()
{
  // store the key presses for this "insert mode session" so that it can be repeated with the
  // '.' command
  if (!m_viInputModeManager->isRunningMacro()) {
    m_viInputModeManager->storeChangeCommand();
    m_viInputModeManager->clearLog();
  }

  m_viInputModeManager->viEnterNormalMode();
  m_view->doc()->setMergeAllEdits(false);
  m_view->updateViModeBarMode();

  return true;
}

bool KateViModeBase::startInsertMode()
{
  m_viInputModeManager->viEnterInsertMode();
  m_view->doc()->setMergeAllEdits(true);
  m_view->updateViModeBarMode();

  return true;
}

bool KateViModeBase::startReplaceMode()
{
  m_view->doc()->setMergeAllEdits(true);
  m_viInputModeManager->viEnterReplaceMode();
  m_view->updateViModeBarMode();

  return true;
}

bool KateViModeBase::startVisualMode()
{
  if ( m_view->getCurrentViMode() == VisualLineMode ) {
    m_viInputModeManager->getViVisualMode()->setVisualLine( false );
    m_viInputModeManager->changeViMode(VisualMode);
  } else if (m_view->getCurrentViMode() == VisualBlockMode ) {
    m_viInputModeManager->getViVisualMode()->setVisualBlock( false );
    m_viInputModeManager->changeViMode(VisualMode);
  } else {
    m_viInputModeManager->viEnterVisualMode();
  }

  m_view->updateViModeBarMode();

  return true;
}

bool KateViModeBase::startVisualBlockMode()
{
  if ( m_view->getCurrentViMode() == VisualMode ) {
    m_viInputModeManager->getViVisualMode()->setVisualBlock( true );
    m_viInputModeManager->changeViMode(VisualBlockMode);
  } else {
    m_viInputModeManager->viEnterVisualMode( VisualBlockMode );
  }

  m_view->updateViModeBarMode();

  return true;
}

bool KateViModeBase::startVisualLineMode()
{
  if ( m_view->getCurrentViMode() == VisualMode ) {
    m_viInputModeManager->getViVisualMode()->setVisualLine( true );
    m_viInputModeManager->changeViMode(VisualLineMode);
  } else {
    m_viInputModeManager->viEnterVisualMode( VisualLineMode );
  }

  m_view->updateViModeBarMode();

  return true;
}

void KateViModeBase::error( const QString &errorMsg ) const
{
  m_view->viModeBar()->showErrorMessage(errorMsg);
}

void KateViModeBase::message( const QString &msg ) const
{
  m_view->viModeBar()->showMessage(msg);
}

QString KateViModeBase::getVerbatimKeys() const
{
  return m_keysVerbatim;
}

void KateViModeBase::addMapping( const QString &from, const QString &to )
{
    m_mappings[from] = to;
}

const QString KateViModeBase::getMapping( const QString &from ) const
{
    return m_mappings[from];
}

const QStringList KateViModeBase::getMappings() const
{
    QStringList l;
    foreach ( const QString &str, m_mappings.keys() ) {
      l << str;
    }

    return l;
}

const QChar KateViModeBase::getCharAtVirtualColumn( QString &line, int virtualColumn,
    int tabWidth ) const
{
  int column = 0;
  int tempCol = 0;

  while ( tempCol < virtualColumn ) {
    if ( line.at( column ) == '\t' ) {
      tempCol += tabWidth - ( tempCol % tabWidth );
    } else {
      tempCol++;
    }

    if ( tempCol <= virtualColumn ) {
      column++;

      if ( column >= line.length() ) {
        return QChar::Null;
      }
    }
  }

  if ( line.length() > column )
    return line.at( column );

  return QChar::Null;
}
