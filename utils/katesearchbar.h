/* ##################################################################
##
##  TODO:
##  * Text/icon area for match/mismatch/wrap indication
##  * Search/replace history
##  * Highlight all with background thread
##  * Fix regex backward search?
##  * Fix match/replacement highlighting?
##  * "Add..." buttons
##  * Proper loading/saving of search settings
##
################################################################## */

/* This file is part of the KDE libraries
   Copyright (C) 2007 SebastianPipping <webmaster@hartwork.org>

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

#ifndef KATE_SEARCH_BAR_H
#define KATE_SEARCH_BAR_H 1

#include "kateviewhelpers.h"
#include "katesmartrange.h"
#include "katedocument.h"



class KateView;
class QVBoxLayout;
class QCheckBox;

namespace Ui {
    class IncrementalSearchBar;
    class PowerSearchBar;
}



class KateSearchBar : public KateViewBarWidget {
    Q_OBJECT

public:
    explicit KateSearchBar(KateViewBar * viewBar);
    ~KateSearchBar();

public Q_SLOTS:
    // Called for <F3> and <Shift>+<F3>
    void findNext();
    void findPrevious();

    void onIncPatternChanged(const QString & pattern);
    void onIncNext();
    void onIncPrev();
    void onStep(bool replace, bool forwards = true);
    void onPowerPatternChanged(const QString & pattern);
    void onPowerFindNext();
    void onPowerFindPrev();
    void onPowerReplaceNext();
    void onPowerReplaceAll();

public Q_SLOTS:
    // Also used by KateView
    void onMutatePower();
    void onMutateIncremental();

private:
    // Helpers
    bool isChecked(QCheckBox * checkbox);
    bool isChecked(QAction * menuAction);
    void enableHighlights(bool enable);
    void resetHighlights();

    void highlightMatch(const KTextEditor::Range & range);
    void highlightReplacement(const KTextEditor::Range & range);
    void indicateMatch(bool wrapped);
    void indicateMismatch();
    void indicateNothing();
    void selectRange(const KTextEditor::Range & range);
    void buildReplacement(QString & output, QList<ReplacementPart> & parts,
            const QVector<KTextEditor::Range> & details);
    void replaceMatch(const QVector<KTextEditor::Range> & match, const QString & replacement);

private:
    // Overridden
    void showEvent(QShowEvent * event);
    void hideEvent(QHideEvent * event);

private:
    // Shared by both dialogs
    KateView * m_view;
    KTextEditor::SmartRange * m_topRange;
    QVBoxLayout * m_layout;
    QWidget * m_widget;

    // Incremental search related
    Ui::IncrementalSearchBar * m_incUi;
    QMenu * m_incMenu;
    QAction * m_incMenuMatchCase;
    QAction * m_incMenuFromCursor;
    QAction * m_incMenuHighlightAll;
    KTextEditor::Cursor m_incInitCursor;

    // Power search related
    Ui::PowerSearchBar * m_powerUi;

};



#endif // KATE_SEARCH_BAR_H

