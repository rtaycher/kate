/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 Anders Lund <anders@alweb.dk>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __KATE_VIEW_HELPERS_H__
#define __KATE_VIEW_HELPERS_H__

#include <klineedit.h>

#include <qwidget.h>
#include <qpixmap.h>
#include <qcolor.h>

class KateDocument;
class KateView;
class KateViewInternal;

class KateCmdLine : public KLineEdit
{
  Q_OBJECT

  public:
    KateCmdLine (KateView *view);

  private slots:
    void slotReturnPressed ( const QString& cmd );
    void hideMe ();

  protected:
    void focusInEvent ( QFocusEvent *ev );
    void keyPressEvent( QKeyEvent *ev );

  private:
    KateView *m_view;
    bool m_msgMode;
    QString m_oldText;
};

class KateIconBorder : public QWidget
{
  Q_OBJECT

  public:
    KateIconBorder( KateViewInternal* internalView, QWidget *parent );

    // VERY IMPORTANT ;)
    virtual QSize sizeHint() const;

    void updateFont();
    int lineNumberWidth() const;

    void setIconBorderOn(     bool enable );
    void setLineNumbersOn(    bool enable );
    void setDynWrapIndicators(int state );
    int dynWrapIndicators()  const { return m_dynWrapIndicators; }
    bool dynWrapIndicatorsOn() const { return m_dynWrapIndicatorsOn; }
    void setFoldingMarkersOn( bool enable );
    void toggleIconBorder()     { setIconBorderOn(     !iconBorderOn() );     }
    void toggleLineNumbers()    { setLineNumbersOn(    !lineNumbersOn() );    }
    void toggleFoldingMarkers() { setFoldingMarkersOn( !foldingMarkersOn() ); }
    bool iconBorderOn()       const { return m_iconBorderOn;     }
    bool lineNumbersOn()      const { return m_lineNumbersOn;    }
    bool foldingMarkersOn()   const { return m_foldingMarkersOn; }

    enum BorderArea { None, LineNumbers, IconBorder, FoldingMarkers };
    BorderArea positionToArea( const QPoint& ) const;

  signals:
    void toggleRegionVisibility( unsigned int );

  private:
    void paintEvent( QPaintEvent* );
    void paintBorder (int x, int y, int width, int height);

    void mousePressEvent( QMouseEvent* );
    void mouseMoveEvent( QMouseEvent* );
    void mouseReleaseEvent( QMouseEvent* );
    void mouseDoubleClickEvent( QMouseEvent* );

    void showMarkMenu( uint line, const QPoint& pos );

    KateView *m_view;
    KateDocument *m_doc;
    KateViewInternal *m_viewInternal;

    bool m_iconBorderOn:1;
    bool m_lineNumbersOn:1;
    bool m_foldingMarkersOn:1;
    bool m_dynWrapIndicatorsOn:1;
    int m_dynWrapIndicators;

    uint m_lastClickedLine;

    int m_cachedLNWidth;

    int m_maxCharWidth;

    mutable QPixmap m_arrow;
    mutable QColor m_oldBackgroundColor;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
