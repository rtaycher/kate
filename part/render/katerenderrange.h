/* This file is part of the KDE libraries
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>

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

#ifndef KATERENDERRANGE_H
#define KATERENDERRANGE_H

#include <ktexteditor/cursor.h>
#include <ktexteditor/attribute.h>

#include <QtCore/QStack>
#include <QtCore/QList>
#include <QtCore/QPair>

class KateSmartRange;
class KateView;

class KateRenderRange
{
  public:
    virtual ~KateRenderRange() {}
    virtual KTextEditor::Cursor nextBoundary() const = 0;
    virtual bool advanceTo(const KTextEditor::Cursor& pos) const = 0;
    virtual KTextEditor::Attribute::Ptr currentAttribute() const = 0;
};

class SmartRenderRange : public KateRenderRange
{
  public:
    SmartRenderRange(KateSmartRange* range, bool useDynamic, KateView* view);

    virtual KTextEditor::Cursor nextBoundary() const;
    virtual bool advanceTo(const KTextEditor::Cursor& pos) const;
    virtual KTextEditor::Attribute::Ptr currentAttribute() const;

  private:
    void addTo(KTextEditor::SmartRange* range) const;

    mutable KTextEditor::SmartRange* m_currentRange;
    mutable KTextEditor::Cursor m_currentPos;
    mutable QStack<KTextEditor::Attribute::Ptr> m_attribs;
    const KateView* m_view;
    const bool m_useDynamic;
};

typedef QPair<KTextEditor::Range*,KTextEditor::Attribute::Ptr> pairRA;

class NormalRenderRange : public KateRenderRange
{
  public:
    NormalRenderRange();
    virtual ~NormalRenderRange();

    void addRange(KTextEditor::Range* range, KTextEditor::Attribute::Ptr attribute);

    virtual KTextEditor::Cursor nextBoundary() const;
    virtual bool advanceTo(const KTextEditor::Cursor& pos) const;
    virtual KTextEditor::Attribute::Ptr currentAttribute() const;

  private:
    mutable QList<pairRA> m_ranges;
    mutable KTextEditor::Cursor m_currentPos;
    mutable int m_currentRange;
};

class RenderRangeList : public QList<KateRenderRange*>
{
  public:
    void appendRanges(const QList<KTextEditor::SmartRange*>& startingRanges, bool useDynamic, KateView* view);
    KTextEditor::Cursor nextBoundary() const;
    bool advanceTo(const KTextEditor::Cursor& pos) const;
    bool hasAttribute() const;
    KTextEditor::Attribute generateAttribute() const;

  private:
    KTextEditor::Cursor m_currentPos;
};

#endif
