/* This file is part of the KDE libraries
   Copyright (C) 2003 Hamish Rodda <meddie@yoyo.its.monash.edu.au>

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

#include "katesupercursor.h"

#include <qobjectlist.h>

#include <kdebug.h>

#include "katetextline.h"
#include "katebuffer.h"
#include "katedocument.h"

#ifdef DEBUGTESTING
static KateSuperCursor* debugCursor = 0L;
static KateSuperRange* debugRange = 0L;
#endif

KateSuperCursor::KateSuperCursor(KateDocument* doc, const KateTextCursor& cursor, QObject* parent, const char* name)
  : QObject(parent, name)
  , KateDocCursor(cursor.line, cursor.col, doc)
  , m_line(doc->kateTextLine(cursor.line))
{
  Q_ASSERT(m_line);

  connectSS();
}

KateSuperCursor::KateSuperCursor(KateDocument* doc, int lineNum, int col, QObject* parent, const char* name)
  : QObject(parent, name)
  , KateDocCursor(lineNum, col, doc)
  , m_line(doc->kateTextLine(lineNum))
{
  Q_ASSERT(m_line);

  connectSS();
}

#ifdef DEBUGTESTING
KateSuperCursor::~KateSuperCursor()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl;
}
#endif

bool KateSuperCursor::atStartOfLine()
{
  return col == 0;
}

bool KateSuperCursor::atEndOfLine()
{
  return col >= (int)m_line->length();
}

bool KateSuperCursor::moveOnInsert() const
{
  return m_moveOnInsert;
}

void KateSuperCursor::setMoveOnInsert(bool moveOnInsert)
{
  m_moveOnInsert = moveOnInsert;
}

void KateSuperCursor::connectSS()
{
  m_moveOnInsert = false;
  m_lineRemoved = false;

#ifdef DEBUGTESTING
  if (!debugCursor) {
    debugCursor = this;
    connect(this, SIGNAL(positionChanged()), SLOT(slotPositionChanged()));
    connect(this, SIGNAL(positionUnChanged()), SLOT(slotPositionUnChanged()));
    connect(this, SIGNAL(positionDeleted()), SLOT(slotPositionDeleted()));
    connect(this, SIGNAL(charInsertedAt()), SLOT(slotCharInsertedAt()));
    connect(this, SIGNAL(charDeletedBefore()), SLOT(slotCharDeletedBefore()));
    connect(this, SIGNAL(charDeletedAfter()), SLOT(slotCharDeletedAfter()));
  }
#endif

  connect(m_line->buffer(), SIGNAL(textInserted(TextLine::Ptr, uint, uint)), SLOT(slotTextInserted(TextLine::Ptr, uint, uint)));
  connect(m_line->buffer(), SIGNAL(lineInsertedBefore(TextLine::Ptr, uint)), SLOT(slotLineInsertedBefore(TextLine::Ptr, uint)));
  connect(m_line->buffer(), SIGNAL(textRemoved(TextLine::Ptr, uint, uint)), SLOT(slotTextRemoved(TextLine::Ptr, uint, uint)));
  connect(m_line->buffer(), SIGNAL(lineRemoved(uint)), SLOT(slotLineRemoved(uint)));
  connect(m_line->buffer(), SIGNAL(textWrapped(TextLine::Ptr, TextLine::Ptr, uint)), SLOT(slotTextWrapped(TextLine::Ptr, TextLine::Ptr, uint)));
  connect(m_line->buffer(), SIGNAL(textUnWrapped(TextLine::Ptr, TextLine::Ptr, uint, uint)), SLOT(slotTextUnWrapped(TextLine::Ptr, TextLine::Ptr, uint, uint)));
}

void KateSuperCursor::slotTextInserted(TextLine::Ptr linePtr, uint pos, uint len)
{
  if (m_line == linePtr) {
    bool insertedAt = col == int(pos);

    if (m_moveOnInsert ? (col > int(pos)) : (col >= int(pos))) {
      col += len;

      if (insertedAt)
        emit charInsertedAt();

      emit positionChanged();
      return;
    }
  }

  emit positionUnChanged();
}

void KateSuperCursor::slotLineInsertedBefore(TextLine::Ptr linePtr, uint lineNum)
{
  // NOTE not >= because the = case is taken care of slotTextWrapped.
  // Comparing to linePtr is because we can get a textWrapped signal then a lineInsertedBefore signal.
  if (m_line != linePtr && line > int(lineNum)) {
    line++;
    emit positionChanged();
    return;

  } else if (m_line != linePtr) {
    emit positionUnChanged();
  }
}

void KateSuperCursor::slotTextRemoved(TextLine::Ptr linePtr, uint pos, uint len)
{
  if (m_line == linePtr) {
    if (col > int(pos)) {
      if (col > int(pos + len)) {
        col -= len;

      } else {
        bool prevCharDeleted = col == int(pos + len);

        col = pos;

        if (prevCharDeleted)
          emit charDeletedBefore();
        else
          emit positionDeleted();
      }

      emit positionChanged();
      return;

    } else if (col == int(pos)) {
      emit charDeletedAfter();
    }
  }

  emit positionUnChanged();
}

// TODO does this work with multiple line deletions?
// TODO does this need the same protection with the actual TextLine::Ptr as lineInsertedBefore?
void KateSuperCursor::slotLineRemoved(uint lineNum)
{
  if (line == int(lineNum)) {
    // They took my line! :(
    bool atStart = col == 0;

    if (m_doc->numLines() <= lineNum) {
      line = m_doc->numLines() - 1;
      m_line = m_doc->kateTextLine(line);
      col = m_line->length();

    } else {
      m_line = m_doc->kateTextLine(line);
      col = 0;
    }

    if (atStart)
      emit charDeletedBefore();
    else
      emit positionDeleted();

    emit positionChanged();
    return;

  } else if (line > int(lineNum)) {
    line--;
    emit positionChanged();
    return;
  }

  if (m_lineRemoved) {
    m_lineRemoved = false;
    return;
  }

  emit positionUnChanged();
}

void KateSuperCursor::slotTextWrapped(TextLine::Ptr linePtr, TextLine::Ptr nextLine, uint pos)
{
  if (m_line == linePtr) {
    if (col >= int(pos)) {
      bool atStart = col == 0;
      col -= pos;
      line++;
      m_line = nextLine;

      if (atStart)
        emit charDeletedBefore();

      emit positionChanged();
      return;
    }
  }

  // This is the job of the lineRemoved / lineInsertedBefore methods.
  //emit positionUnChanged();
}

void KateSuperCursor::slotTextUnWrapped(TextLine::Ptr linePtr, TextLine::Ptr nextLine, uint pos, uint len)
{
  if (m_line == nextLine) {
    if (col < int(len)) {
      col += pos;
      line--;
      m_line = linePtr;
      emit positionChanged();
      m_lineRemoved = true;
      return;
    }
  }

  // This is the job of the lineRemoved / lineInsertedBefore methods.
  //emit positionUnChanged();
}

#ifdef DEBUGTESTING
void KateSuperCursor::slotPositionChanged()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl/* << kdBacktrace(7)*/;
}

void KateSuperCursor::slotPositionUnChanged()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl/* << kdBacktrace(7)*/;
}

void KateSuperCursor::slotPositionDeleted()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperCursor::slotCharInsertedAt()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperCursor::slotCharDeletedBefore()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperCursor::slotCharDeletedAfter()
{
  if (debugCursor == this) kdDebug() << k_funcinfo << endl;
}
#endif

KateSuperCursor::operator QString()
{
  return QString("[%1,%1]").arg(line).arg(col);
}

KateSuperRange::KateSuperRange(KateSuperCursor* start, KateSuperCursor* end, QObject* parent, const char* name)
  : QObject(parent, name)
  , m_start(start)
  , m_end(end)
  , m_evaluate(false)
  , m_startChanged(false)
  , m_endChanged(false)
{
  Q_ASSERT(isValid());

  insertChild(m_start);
  insertChild(m_end);

  setBehaviour(DoNotExpand);

  m_evaluate = false;
  m_startChanged = false;
  m_endChanged = false;

#ifdef DEBUGTESTING
  if (!debugRange) {
    debugRange = this;
    connect(this, SIGNAL(positionChanged()), SLOT(slotPositionChanged()));
    connect(this, SIGNAL(positionUnChanged()), SLOT(slotPositionUnChanged()));
    connect(this, SIGNAL(contentsChanged()), SLOT(slotContentsChanged()));
    connect(this, SIGNAL(boundaryDeleted()), SLOT(slotBoundaryDeleted()));
    connect(this, SIGNAL(eliminated()), SLOT(slotEliminated()));
  }
#endif

  connect(m_start, SIGNAL(positionChanged()),  SLOT(slotEvaluateChanged()));
  connect(m_end, SIGNAL(positionChanged()),  SLOT(slotEvaluateChanged()));
  connect(m_start, SIGNAL(positionUnChanged()), SLOT(slotEvaluateUnChanged()));
  connect(m_end, SIGNAL(positionUnChanged()), SLOT(slotEvaluateUnChanged()));
  connect(m_start, SIGNAL(positionDeleted()), SIGNAL(boundaryDeleted()));
  connect(m_end, SIGNAL(positionDeleted()), SIGNAL(boundaryDeleted()));
}

#ifdef DEBUGTESTING
KateSuperRange::~KateSuperRange()
{
  if (debugRange == this) kdDebug() << k_funcinfo << endl;
}
#endif

KateSuperCursor& KateSuperRange::start() const
{
  return *m_start;
}

KateSuperCursor& KateSuperRange::end() const
{
  return *m_end;
}

int KateSuperRange::behaviour() const
{
  return (m_start->moveOnInsert() ? DoNotExpand : ExpandLeft) | (m_end->moveOnInsert() ? ExpandRight : DoNotExpand);
}

void KateSuperRange::setBehaviour(int behaviour)
{
  m_start->setMoveOnInsert(behaviour & ExpandLeft);
  m_end->setMoveOnInsert(!(behaviour & ExpandRight));
}

bool KateSuperRange::isValid() const
{
  return start() <= end();
}

bool KateSuperRange::owns(const KateTextCursor& cursor) const
{
  if (!includes(cursor)) return false;

  if (children())
    for (QObjectListIt it(*children()); *it; ++it)
      if ((*it)->inherits("KateSuperRange"))
        if (static_cast<KateSuperRange*>(*it)->owns(cursor))
          return false;

  return true;
}

bool KateSuperRange::includes(const KateTextCursor& cursor) const
{
  return isValid() && cursor >= start() && cursor < end();
}

bool KateSuperRange::includes(uint line) const
{
  return isValid() && (int)line >= m_start->line && (int)line <= m_end->line;
}

bool KateSuperRange::includesWholeLine(uint line) const
{
  return isValid() && ((int)line > m_start->line || ((int)line == m_start->line && m_start->atStartOfLine())) && ((int)line < m_end->line || ((int)line == m_end->line && m_end->atEndOfLine()));
}

bool KateSuperRange::boundaryAt(const KateTextCursor& cursor) const
{
  return isValid() && (cursor == start() || cursor == end());
}

bool KateSuperRange::boundaryOn(uint line) const
{
  return isValid() && (m_start->line == (int)line || m_end->line == (int)line);
}

void KateSuperRange::slotEvaluateChanged()
{
  if (sender() == static_cast<QObject*>(m_start)) {
    if (m_evaluate) {
      if (!m_endChanged) {
        // Only one was changed
        evaluateEliminated();

      } else {
        // Both were changed
        evaluatePositionChanged();
        m_endChanged = false;
      }

    } else {
      m_startChanged = true;
    }

  } else {
    if (m_evaluate) {
      if (!m_startChanged) {
        // Only one was changed
        evaluateEliminated();

      } else {
        // Both were changed
        evaluatePositionChanged();
        m_startChanged = false;
      }

    } else {
      m_endChanged = true;
    }
  }

  m_evaluate = !m_evaluate;
}

void KateSuperRange::slotEvaluateUnChanged()
{
  if (sender() == static_cast<QObject*>(m_start)) {
    if (m_evaluate) {
      if (m_endChanged) {
        // Only one changed
        evaluateEliminated();
        m_endChanged = false;

      } else {
        // Neither changed
        emit positionUnChanged();
      }
    }

  } else {
    if (m_evaluate) {
      if (m_startChanged) {
        // Only one changed
        evaluateEliminated();
        m_startChanged = false;

      } else {
        // Neither changed
        emit positionUnChanged();
      }
    }
  }

  m_evaluate = !m_evaluate;
}

void KateSuperRange::evaluateEliminated()
{
  if (start() == end())
    emit eliminated();
  else
    emit contentsChanged();
}

void KateSuperRange::evaluatePositionChanged()
{
  if (start() == end())
    emit eliminated();
  else
    emit positionChanged();
}

#ifdef DEBUGTESTING
void KateSuperRange::slotPositionChanged()
{
  if (debugRange == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperRange::slotPositionUnChanged()
{
  if (debugRange == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperRange::slotContentsChanged()
{
  if (debugRange == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperRange::slotBoundaryDeleted()
{
  if (debugRange == this) kdDebug() << k_funcinfo << endl;
}

void KateSuperRange::slotEliminated()
{
  if (debugRange == this) kdDebug() << k_funcinfo << endl;
}
#endif

int KateSuperCursorList::compareItems(QPtrCollection::Item item1, QPtrCollection::Item item2)
{
  if (*(static_cast<KateSuperCursor*>(item1)) == *(static_cast<KateSuperCursor*>(item2)))
    return 0;

  return *(static_cast<KateSuperCursor*>(item1)) < *(static_cast<KateSuperCursor*>(item2)) ? -1 : 1;
}

KateSuperRangeList::KateSuperRangeList(bool autoManage, QObject* parent, const char* name)
  : QObject(parent, name)
  , m_autoManage(autoManage)
  , m_connect(true)
{
  setAutoManage(autoManage);
}

KateSuperRangeList::KateSuperRangeList(const QPtrList<KateSuperRange>& rangeList, QObject* parent, const char* name)
  : QObject(parent, name)
  , m_autoManage(false)
  , m_connect(false)
  , m_trackingBoundaries(false)
{
  appendList(rangeList);
}

void KateSuperRangeList::appendList(const QPtrList<KateSuperRange>& rangeList)
{
  for (QPtrListIterator<KateSuperRange> it = rangeList; *it; ++it)
    append(*it);
}

void KateSuperRangeList::clear()
{
  for (KateSuperRange* range = first(); range; range = next())
    emit rangeEliminated(range);

  QPtrList<KateSuperRange>::clear();
}

void KateSuperRangeList::connectAll()
{
  if (!m_connect) {
    m_connect = true;
    for (KateSuperRange* range = first(); range; range = next()) {
      connect(range, SIGNAL(destroyed(QObject*)), SLOT(slotDeleted(QObject*)));
      connect(range, SIGNAL(eliminated()), SLOT(slotEliminated()));
    }
  }
}

bool KateSuperRangeList::autoManage() const
{
  return m_autoManage;
}

void KateSuperRangeList::setAutoManage(bool autoManage)
{
  m_autoManage = autoManage;
  setAutoDelete(m_autoManage);
}

QPtrList<KateSuperRange> KateSuperRangeList::rangesIncluding(const KateTextCursor& cursor)
{
  sort();

  QPtrList<KateSuperRange> ret;

  for (KateSuperRange* r = first(); r; r = next())
    if (r->includes(cursor))
      ret.append(r);

  return ret;
}

QPtrList<KateSuperRange> KateSuperRangeList::rangesIncluding(uint line)
{
  sort();

  QPtrList<KateSuperRange> ret;

  for (KateSuperRange* r = first(); r; r = next())
    if (r->includes(line))
      ret.append(r);

  return ret;
}

bool KateSuperRangeList::rangesInclude(const KateTextCursor& cursor)
{
  for (KateSuperRange* r = first(); r; r = next())
    if (r->includes(cursor))
      return true;

  return false;
}

void KateSuperRangeList::slotEliminated()
{
  if (sender()) {
    KateSuperRange* range = static_cast<KateSuperRange*>(const_cast<QObject*>(sender()));
    emit rangeEliminated(range);

    if (m_trackingBoundaries) {
      m_columnBoundaries.removeRef(&(range->start()));
      m_columnBoundaries.removeRef(&(range->end()));
    }

    if (m_autoManage)
      removeRef(range);

    if (!count())
      emit listEmpty();
  }
}

void KateSuperRangeList::slotDeleted(QObject* range)
{
  KateSuperRange* r = static_cast<KateSuperRange*>(range);
  int index = findRef(r);
  if (index != -1)
    take(index);

  emit rangeDeleted(r);

  if (!count())
      emit listEmpty();
}

KateSuperCursor* KateSuperRangeList::firstBoundary(const KateTextCursor* start)
{
  if (!m_trackingBoundaries) {
    m_trackingBoundaries = true;

    for (KateSuperRange* r = first(); r; r = next()) {
      m_columnBoundaries.append(&(r->start()));
      m_columnBoundaries.append(&(r->end()));
    }
  }

  m_columnBoundaries.sort();

  if (start)
    // OPTIMISE: QMap with QPtrList for each line? (==> sorting issues :( )
    for (KateSuperCursor* c = m_columnBoundaries.first(); c; c = m_columnBoundaries.next())
      if (*start <= *c)
        break;

  return m_columnBoundaries.current();
}

KateSuperCursor* KateSuperRangeList::nextBoundary()
{
  KateSuperCursor* current = m_columnBoundaries.current();

  // make sure the new cursor is after the current cursor; multiple cursors with the same position can be in the list.
  if (current)
    while (m_columnBoundaries.next())
      if (*(m_columnBoundaries.current()) != *current)
        break;

  return m_columnBoundaries.current();
}

KateSuperCursor* KateSuperRangeList::currentBoundary()
{
  return m_columnBoundaries.current();
}

int KateSuperRangeList::compareItems(QPtrCollection::Item item1, QPtrCollection::Item item2)
{
  if (static_cast<KateSuperRange*>(item1)->start() == static_cast<KateSuperRange*>(item2)->start()) {
    if (static_cast<KateSuperRange*>(item1)->end() == static_cast<KateSuperRange*>(item2)->end()) {
      return 0;
    } else {
      return static_cast<KateSuperRange*>(item1)->end() < static_cast<KateSuperRange*>(item2)->end() ? -1 : 1;
    }
  }

  return static_cast<KateSuperRange*>(item1)->start() < static_cast<KateSuperRange*>(item2)->start() ? -1 : 1;
}

QPtrCollection::Item KateSuperRangeList::newItem(QPtrCollection::Item d)
{
  if (m_connect) {
    connect(static_cast<KateSuperRange*>(d), SIGNAL(destroyed(QObject*)), SLOT(slotDeleted(QObject*)));
    connect(static_cast<KateSuperRange*>(d), SIGNAL(eliminated()), SLOT(slotEliminated()));
  }

  if (m_trackingBoundaries) {
    m_columnBoundaries.append(&(static_cast<KateSuperRange*>(d)->start()));
    m_columnBoundaries.append(&(static_cast<KateSuperRange*>(d)->end()));
  }

  return QPtrList<KateSuperRange>::newItem(d);
}

#include "katesupercursor.moc"
