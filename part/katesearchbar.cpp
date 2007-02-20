/* This file is part of the KDE libraries
   Copyright (C) 2006 Andreas Kling <kling@impul.se>

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

#include "katesearchbar.h"
#include "katedocument.h"
#include "kateview.h"
#include "kateviewinternal.h"
#include "katesmartrange.h"

#include <kglobalsettings.h>
#include <kiconloader.h>

#include <QHBoxLayout>
#include <QToolButton>
#include <QCheckBox>
#include <QKeyEvent>
#include <QList>

#include <kdebug.h>

class KateSearchBar::Private
{
public:
    KTextEditor::Cursor startCursor;
    KTextEditor::Range match;
    KTextEditor::Range lastMatch;
    KateSearchBarEdit *expressionEdit;

    QCheckBox *caseSensitiveBox;
    QCheckBox *wholeWordsBox;
    QCheckBox *regExpBox;

    QCheckBox *fromCursorBox;
    QCheckBox *selectionOnlyBox;
    QCheckBox *highlightAllBox;

    QRegExp regExp;

    bool searching;
    bool wrapAround;

    QList<KTextEditor::SmartRange*> allMatches;
};

KateSearchBar::KateSearchBar(KateViewBar *viewBar)
    : KateViewBarWidget (viewBar),
      m_view(viewBar->view()),
      d(new Private)
{
    centralWidget()->setFont(KGlobalSettings::toolBarFont());

    d->searching = false;
    d->wrapAround = true;

    d->expressionEdit = new KateSearchBarEdit;
    connect(d->expressionEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotSearch()));
    connect(d->expressionEdit, SIGNAL(returnPressed()), this, SLOT(slotSearch()));
    connect(d->expressionEdit, SIGNAL(findNext()), this, SLOT(findNext()));
    connect(d->expressionEdit, SIGNAL(findPrevious()), this, SLOT(findPrevious()));

    d->caseSensitiveBox = new QCheckBox(i18n("&Case Sensitive"));
    connect(d->caseSensitiveBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    d->wholeWordsBox = new QCheckBox(i18n("&Whole Words"));
    connect(d->wholeWordsBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    d->regExpBox = new QCheckBox(i18n("&Regular Expression"));
    connect(d->regExpBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    d->fromCursorBox = new QCheckBox(i18n("&From Cursor"));
    connect(d->fromCursorBox, SIGNAL(stateChanged(int)), this, SLOT(slotSearch()));

    d->selectionOnlyBox = new QCheckBox(i18n("&Selection Only"));
    connect(d->selectionOnlyBox, SIGNAL(stateChanged(int)), this, SLOT(slotSpecialOptionTogled()));

    d->highlightAllBox = new QCheckBox(i18n("&Highlight all"));
    connect(d->highlightAllBox, SIGNAL(stateChanged(int)), this, SLOT(slotSpecialOptionTogled()));

    QVBoxLayout *topLayout = new QVBoxLayout ();
    centralWidget()->setLayout(topLayout);

    // NOTE: Here be cosmetics.
    topLayout->setMargin(2);


    // first line, search field + next/prev
    QToolButton *nextButton = new QToolButton();
    nextButton->setAutoRaise(true);
    nextButton->setIcon(SmallIcon("next"));
    nextButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    nextButton->setText(i18n("Next"));
    connect(nextButton, SIGNAL(clicked()), this, SLOT(findNext()));

    QToolButton *prevButton = new QToolButton();
    prevButton->setAutoRaise(true);
    prevButton->setIcon(SmallIcon("previous"));
    prevButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    prevButton->setText(i18n("Previous"));
    connect(prevButton, SIGNAL(clicked()), this, SLOT(findPrevious()));

    QHBoxLayout *layoutFirstLine = new QHBoxLayout;
    topLayout->addLayout (layoutFirstLine);

    // first line: lineedit + next + prev
    layoutFirstLine->addWidget(d->expressionEdit);
    layoutFirstLine->addWidget(prevButton);
    layoutFirstLine->addWidget(nextButton);

    QGridLayout *gridLayout = new QGridLayout;
    topLayout->addLayout (gridLayout);

    // second line: casesensitive + whole words + regexp
    gridLayout->addWidget(d->caseSensitiveBox, 0, 0);
    gridLayout->addWidget(d->wholeWordsBox, 0, 1);
    gridLayout->addWidget(d->regExpBox, 0, 2);
    gridLayout->addItem (new QSpacerItem(0,0), 0, 3);

    // third line: from cursor + selection only + highlight all
    gridLayout->addWidget(d->fromCursorBox, 1, 0);
    gridLayout->addWidget(d->selectionOnlyBox, 1, 1);
    gridLayout->addWidget(d->highlightAllBox, 1, 2);
    gridLayout->addItem (new QSpacerItem(0,0), 1, 3);

    gridLayout->setColumnStretch (3, 10);

    // set right focus proxy
    centralWidget()->setFocusProxy(d->expressionEdit);
}

KateSearchBar::~KateSearchBar()
{
    delete d;
}

void KateSearchBar::doSearch(const QString &_expression, bool init, bool backwards)
{
    QString expression = _expression;

    bool selected = d->selectionOnlyBox->checkState();

    // If we're starting search for the first time, begin at the current cursor position.
    // ### there may be cases when this should happen, but does not
    if ( init )
    {
        d->startCursor = d->fromCursorBox->checkState() ==
            Qt::Checked ? m_view->cursorPosition() :
              selected ? m_view->selectionRange().start() :
              KTextEditor::Cursor( 0, 0 );
        d->searching = true;

        // clear
        while ( ! d->allMatches.isEmpty() )
        {
          KTextEditor::SmartRange* sr = d->allMatches.takeFirst();
          kDebug() << "remove highlight: ["<<sr->start()<<"] - ["<<sr->end()<<"]"<<endl;
          m_view->removeInternalHighlight( sr );
          delete sr;
        }
    }

    if ( d->regExpBox->checkState() != Qt::Checked )
      expression = QRegExp::escape( expression );

    if ( d->wholeWordsBox->checkState() == Qt::Checked )
        expression = "\\b" + expression + "\\b";

    d->regExp.setPattern(expression);

    if ( ! d->regExp.isValid() )
        return;

    d->regExp.setCaseSensitivity(d->caseSensitiveBox->checkState() == Qt::Checked ? Qt::CaseSensitive : Qt::CaseInsensitive);


    d->match = m_view->doc()->searchText(d->startCursor, d->regExp, backwards);

    bool foundMatch = d->match.isValid() && d->match != d->lastMatch && d->match != m_view->selectionRange();
    bool wrapped = false;

    if (d->wrapAround && !foundMatch)
    {
        // We found nothing, so wrap.
        d->startCursor = selected?
            backwards ? m_view->selectionRange().end() : m_view->selectionRange().start()
        :
            backwards ? m_view->doc()->documentEnd() : KTextEditor::Cursor(0, 0);
        d->match = m_view->doc()->searchText(d->startCursor, d->regExp, backwards);
        foundMatch = d->match.isValid() && d->match != d->lastMatch;
        wrapped = true;
    }

    if ( foundMatch && selected )
        foundMatch = m_view->selectionRange().contains( d->match );

    if (foundMatch)
    {
        m_view->setCursorPositionInternal(d->match.start(), 1);
        m_view->setSelection(d->match);
        d->lastMatch = d->match;
        // it makes no sense to have this enabled after a match
        if ( selected )
            d->selectionOnlyBox->setCheckState( Qt::Unchecked );

        // highlight all matches
        if ( d->highlightAllBox->checkState() == Qt::Checked )
        {

          // add highlight to the current match
          d->allMatches.prepend( m_view->doc()->newSmartRange( d->match ) );

          d->startCursor = d->match.end();

          // find other matches and put each in a smartrange
          bool another = true;
          do {
            KTextEditor::Range r = m_view->doc()->searchText( d->startCursor, d->regExp, false );
            if ( r.isValid() )
            {
                d->allMatches.append( m_view->doc()->newSmartRange( r ) );
                d->startCursor = r.end();
                kDebug()<<"MATCH: "<<r<<endl;
            }

            else if ( ! wrapped )
            {
              d->startCursor = backwards ? m_view->doc()->documentEnd() : KTextEditor::Cursor(0, 0);
              wrapped = true;
            }

            another = r.isValid();
          } while (another);

          // add highlight to each range
          KTextEditor::Attribute::Ptr a( new KTextEditor::Attribute() );
          a->setBackground( QColor("yellow") ); //TODO make this part of the color scheme
          foreach( KTextEditor::SmartRange* sr, d->allMatches )
          {
            sr->setAttribute( a );
            m_view->addInternalHighlight( sr );
            kDebug()<<"added highlight to match ["<<sr->start()<<"] - ["<<sr->end()<<"]"<<endl;
          }

        }
    }


    d->expressionEdit->setStatus(foundMatch ? (wrapped ? KateSearchBarEdit::SearchWrapped : KateSearchBarEdit::Normal) : KateSearchBarEdit::NotFound);
}

void KateSearchBar::slotSpecialOptionTogled()
{
  if ( d->selectionOnlyBox->checkState() == Qt::Checked || d->highlightAllBox->checkState() == Qt::Checked )
      disconnect(d->expressionEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotSearch()));
  else
      connect(d->expressionEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotSearch()));

  d->expressionEdit->setFocus( Qt::OtherFocusReason );
}

void KateSearchBarEdit::setStatus(Status status)
{
    m_status = status;

    QPalette pal;
    QColor col;
    switch (status)
    {
        case KateSearchBarEdit::Normal:
            col = QPalette().color(QPalette::Base);
            break;
        case KateSearchBarEdit::NotFound:
            col = QColor("lightsalmon");
            break;
        case KateSearchBarEdit::SearchWrapped:
            col = QColor("palegreen");
            break;
    }
    pal.setColor(QPalette::Base, col);
    setPalette(pal);
}

void KateSearchBar::slotSearch()
{
    // ### can we achieve this in a better way?
    if ( isVisible() )
        d->expressionEdit->setFocus( Qt::OtherFocusReason );

    if (d->expressionEdit->text().isEmpty())
    {
        kDebug()<<"reset!!"<<endl;
        d->expressionEdit->setStatus(KateSearchBarEdit::Normal);
        d->searching = false;
    }
    else
        doSearch(d->expressionEdit->text(), d->highlightAllBox->checkState()==Qt::Checked||d->selectionOnlyBox->checkState()==Qt::Checked||!d->searching);

    if ( d->expressionEdit->status() == KateSearchBarEdit::NotFound)
    {
        // nothing found, so select the non-matching part of the text
        if ( d->expressionEdit->text().startsWith( m_view->document()->text(d->lastMatch) ) )
            d->expressionEdit->setSelection( d->lastMatch.columnWidth(), d->expressionEdit->text().length() );
        else
            d->expressionEdit->selectAll();
    }

}

void KateSearchBar::findNext()
{
    if (d->lastMatch.isEmpty())
        return;
    d->startCursor = d->lastMatch.end();
    doSearch(d->expressionEdit->text(), false, false);
}

void KateSearchBar::findPrevious()
{
    if (d->lastMatch.isEmpty())
        return;
    d->startCursor = d->lastMatch.start();
    doSearch(d->expressionEdit->text(), false, true);
}

void KateSearchBar::showEvent(QShowEvent *)
{
    d->searching = false;
    if ( d->selectionOnlyBox->checkState() == Qt::Checked && ! m_view->selection() )
      d->selectionOnlyBox->setCheckState( Qt::Unchecked );
}

KateSearchBarEdit::KateSearchBarEdit(QWidget *parent)
    : KLineEdit(parent)
{
  m_status = Normal;
}

// NOTE: void keyPressEvent(QKeyEvent* ev) does not grab Qt::Key_Tab.
// No idea, why... but that's probably why we use ::event here.
bool KateSearchBarEdit::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);
        switch (ke->key())
        {
            case Qt::Key_Tab:
                emit findNext();
                return true;
            case Qt::Key_Backtab:
                emit findPrevious();
                return true;
            case Qt::Key_Return:
            case Qt::Key_Enter:
                emit returnPressed();
                return true;
        }
    }
    return KLineEdit::event(e);
}

void KateSearchBarEdit::showEvent(QShowEvent *)
{
    selectAll();
}

#include "katesearchbar.moc"
