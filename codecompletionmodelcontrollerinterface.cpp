/* This file is part of the KDE libraries
   Copyright (C) 2008 Niko Sams <niko.sams@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "codecompletionmodelcontrollerinterface.h"

#include <QtCore/QModelIndex>

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

namespace KTextEditor {

CodeCompletionModelControllerInterface::CodeCompletionModelControllerInterface()
{
}

CodeCompletionModelControllerInterface::~CodeCompletionModelControllerInterface()
{
}

bool CodeCompletionModelControllerInterface::shouldStartCompletion(View* view, const QString &insertedText, bool userInsertion, const Cursor &position)
{
    Q_UNUSED(view);
    Q_UNUSED(position);
    if(insertedText.isEmpty())
        return false;

    QChar lastChar = insertedText.at(insertedText.count() - 1);
    if ((userInsertion && (lastChar.isLetter() || lastChar.isNumber() || lastChar == '_')) ||
        lastChar == '.' || insertedText.endsWith(QLatin1String("->"))) {
        return true;
    }
    return false;
}

Range CodeCompletionModelControllerInterface::completionRange(View* view, const Cursor &position)
{
    Cursor end = position;

    QString text = view->document()->line(end.line());

    static QRegExp findWordStart( "\\b([_\\w]+)$" );
    static QRegExp findWordEnd( "^([_\\w]*)\\b" );

    Cursor start = end;

    if (findWordStart.lastIndexIn(text.left(end.column())) >= 0)
        start.setColumn(findWordStart.pos(1));

    if (findWordEnd.indexIn(text.mid(end.column())) >= 0)
        end.setColumn(end.column() + findWordEnd.cap(1).length());

    return Range(start, end);
}

void CodeCompletionModelControllerInterface::updateCompletionRange(View* view, SmartRange& range)
{
    Q_UNUSED(view);
    if(!range.text().isEmpty() && range.text().count() == 1 && range.text().first().trimmed().isEmpty())
      //When inserting a newline behind an empty completion-range,, move the range forward to its end
      range.start() = range.end();
}

QString CodeCompletionModelControllerInterface::filterString(View* view, const SmartRange &range, const Cursor &position)
{
    return view->document()->text(KTextEditor::Range(range.start(), position));
}

bool CodeCompletionModelControllerInterface::shouldAbortCompletion(View* view, const SmartRange &range, const QString &currentCompletion)
{
    if(view->cursorPosition() < range.start() || view->cursorPosition() > range.end())
      return true; //Always abort when the completion-range has been left
    //Do not abort completions when the text has been empty already before and a newline has been instead
    
    Q_UNUSED(view);
    Q_UNUSED(range);
    static const QRegExp allowedText("^(\\w*)");
    return !allowedText.exactMatch(currentCompletion);
}

void CodeCompletionModelControllerInterface::aborted(KTextEditor::View* view) {
    Q_UNUSED(view);
}

bool CodeCompletionModelControllerInterface::shouldExecute(const QModelIndex& index, QChar inserted) {
  Q_UNUSED(index);
  Q_UNUSED(inserted);
  return false;
}

KTextEditor::CodeCompletionModelControllerInterface2::MatchReaction CodeCompletionModelControllerInterface2::matchingItem(const QModelIndex& selected) {
  return HideListIfAutomaticInvocation;
}

}
