/* This file is part of the KDE libraries
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

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

#ifndef KATESMARTMANAGER_H
#define KATESMARTMANAGER_H

#include <QObject>
#include <QSet>
#include <QLinkedList>

#include <ktexteditor/smartrange.h>

#include "kateedit.h"

class KateDocument;
class KateSmartCursor;
class KateSmartRange;
class KateSmartGroup;

/**
 * Manages SmartCursors and SmartRanges.
 *
 * \todo potential performance optimisation: use separate sets for internal and non-internal cursors + ranges
 * \todo potential performance optimisation: bypass unhooking routines when clearing
 */
class KateSmartManager : public QObject
{
  Q_OBJECT

  public:
    KateSmartManager(KateDocument* parent);
    virtual ~KateSmartManager();

    KateDocument* doc() const;

    /**
     * The process of clearing works as follows:
     * - SmartCursors and SmartRanges emit the relevant signals as usual
     * - The smart manager takes care of deleting the ranges and the cursors
     *   which are not bound to ranges (those cursors get deleted by the ranges
     *   themselves)
     * - isClearing() is set to true while cursors only are being deleted (not cursors as part of a range)
     * - The smart manager takes care of cleaning its internal lists of cursors it deletes
     *   (deleted SmartCursors should not tell the smart manager that they have
     *   been deleted, ie when isClearing() is true)
     */
    inline bool isClearing() const { return m_clearing; }
    void clear(bool includingInternal);

    KateSmartCursor* newSmartCursor(const KTextEditor::Cursor& position, bool moveOnInsert = true, bool internal = true);
    void deleteCursors(bool includingInternal);

    KateSmartRange* newSmartRange(const KTextEditor::Range& range, KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviors insertBehavior = KTextEditor::SmartRange::DoNotExpand, bool internal = true);
    KateSmartRange* newSmartRange(KateSmartCursor* start, KateSmartCursor* end, KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviors insertBehavior = KTextEditor::SmartRange::DoNotExpand, bool internal = true);
    void unbindSmartRange(KTextEditor::SmartRange* range);
    void deleteRanges(bool includingInternal);

    void rangeGotParent(KateSmartRange* range);
    void rangeLostParent(KateSmartRange* range);
    /// This is called regardless of whether a range was deleted via clear(), or deleteRanges(), or otherwise.
    void rangeDeleted(KateSmartRange* range);

    KateSmartGroup* groupForLine(int line) const;

  Q_SIGNALS:
    void signalRangeDeleted(KateSmartRange* range);

  private Q_SLOTS:
    void slotTextChanged(KateEditInfo* edit);
    void verifyCorrect() const;

  private:
    KateSmartRange* feedbackRange(const KateEditInfo& edit, KateSmartRange* range);

    void debugOutput() const;

    KateSmartGroup* m_firstGroup;
    QSet<KateSmartRange*> m_topRanges;
    KateSmartGroup* m_invalidGroup;
    bool m_clearing;
};

/**
 * This class holds a ground of cursors and ranges which involve a certain
 * number of lines in a document.  It allows us to optimise away having to
 * iterate though every single cursor and range when anything changes in the
 * document.
 *
 * Needs a comprehensive regression and performance test suite...
 */
class KateSmartGroup
{
  public:
    KateSmartGroup(int startLine, int endLine, KateSmartGroup* previous, KateSmartGroup* next);

    inline int startLine() const { return m_startLine; }
    inline int newStartLine() const { return m_newStartLine; }
    inline int endLine() const { return m_endLine; }
    inline int newEndLine() const { return m_newEndLine; }
    inline void setEndLine(int endLine) { m_newEndLine = m_endLine = endLine; }
    inline void setNewEndLine(int endLine) { m_newEndLine = endLine; }
    inline int length() const { return m_endLine - m_startLine + 1; }
    inline bool containsLine(int line) const { return line >= m_newStartLine && line <= m_newEndLine; }

    inline KateSmartGroup* previous() const { return m_previous; }
    inline void setPrevious(KateSmartGroup* previous) { m_previous = previous; }

    inline KateSmartGroup* next() const { return m_next; }
    inline void setNext(KateSmartGroup* next) { m_next = next; }

    void addCursor(KateSmartCursor* cursor);
    void changeCursorFeedback(KateSmartCursor* cursor);
    void removeCursor(KateSmartCursor* cursor);
    // Cursors requiring position change feedback
    const QSet<KateSmartCursor*>& feedbackCursors() const;
    // Cursors not requiring feedback
    const QSet<KateSmartCursor*>& normalCursors() const;

    // Cursor movement!
    /**
     * A cursor has joined this group.
     *
     * The cursor already has its new position.
     */
    void joined(KateSmartCursor* cursor);

    /**
     * A cursor is leaving this group.
     *
     * The cursor still has its old position.
     */
    void leaving(KateSmartCursor* cursor);

    // Merge with the next Smart Group, because this group became too small.
    void merge();

    // First pass for translation
    void translateChanged(const KateEditInfo& edit);
    void translateShifted(const KateEditInfo& edit);

    // Second pass for feedback
    void translatedChanged(const KateEditInfo& edit);
    void translatedShifted(const KateEditInfo& edit);

    void deleteCursors(bool includingInternal);
    void deleteCursorsInternal(QSet<KateSmartCursor*>& set);

    void debugOutput() const;

  private:
    int m_startLine, m_newStartLine, m_endLine, m_newEndLine;
    KateSmartGroup* m_next;
    KateSmartGroup* m_previous;

    QSet<KateSmartCursor*> m_feedbackCursors;
    QSet<KateSmartCursor*> m_normalCursors;
};

#endif
