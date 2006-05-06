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

#include "katedynamicanimation.h"

#include "kateview.h"
#include "katedocument.h"
#include "katerenderer.h"

#include <QTimer>

using namespace KTextEditor;

static const int s_granularity = 4;

KateDynamicAnimation::KateDynamicAnimation(KateDocument* doc, KateSmartRange * range, Attribute::ActivationType type)
 : QObject(doc)
 , m_range(range)
 , m_type(type)
 , m_timer(new QTimer(this))
 , m_sequence(0)
{
  Q_ASSERT(dynamicAttribute());
  init();
}

KateDynamicAnimation::KateDynamicAnimation(KateView* view, KateSmartRange* range, Attribute::ActivationType type)
 : QObject(view)
 , m_range(range)
 , m_type(type)
 , m_timer(new QTimer(this))
 , m_sequence(0)
{
  Q_ASSERT(dynamicAttribute());
  init();
}

KateDynamicAnimation::~KateDynamicAnimation()
{
  if (m_range) {
    m_range->removeDynamic(this);

    if (view())
      view()->renderer()->dynamicRegion().removeRange(m_range);
    else
      foreach (KDocument::View* view, document()->views())
        static_cast<KateView*>(view)->renderer()->dynamicRegion().removeRange(m_range);
  }
}

void KateDynamicAnimation::init( )
{
  if (!dynamicAttribute()) {
    kDebug() << k_funcinfo << "No dynamic attribute for range " << *m_range << endl;
    return;
  }

  connect(m_timer, SIGNAL(timeout()), SLOT(timeout()));

  Attribute::Effects effects = dynamicAttribute()->effects();
  if (effects & Attribute::EffectFadeIn) {
    // Sequence starts at 0
  } else {
    m_sequence = 100;
  }

  m_range->addDynamic(this);

  m_timer->setInterval(25);
  m_timer->start();
}

KateSmartRange * KateDynamicAnimation::range( ) const
{
  return m_range;
}

KateDocument* KateDynamicAnimation::document() const
{
  return qobject_cast<KateDocument*>(const_cast<QObject*>(parent()));
}

KateView* KateDynamicAnimation::view( ) const
{
  return qobject_cast<KateView*>(const_cast<QObject*>(parent()));
}

KTextEditor::Attribute::Ptr KateDynamicAnimation::dynamicAttribute( ) const
{
  return m_range && m_range->attribute() ? m_range->attribute()->dynamicAttribute(m_type) : KTextEditor::Attribute::Ptr();
}

void KateDynamicAnimation::timeout()
{
  if (!m_range) {
    deleteLater();
    return;
  }

  m_sequence += s_granularity;

  //kDebug() << k_funcinfo << *m_range << " Seq " << m_sequence << endl;

  emit redraw(m_range);

  if (m_sequence == 100) {
      m_timer->stop();
  }

  if (m_sequence >= 300) {
    m_timer->stop();
    deleteLater();
  }
}

void KateDynamicAnimation::mergeToAttribute( KTextEditor::Attribute::Ptr attrib ) const
{
  if (!dynamicAttribute()) {
    m_timer->stop();
    return;
  }

  Attribute::Effects effects = dynamicAttribute()->effects();

  //kDebug() << k_funcinfo << m_sequence << "Effects: " << effects << endl;

  if (m_sequence > 0 && m_sequence < 100) {
    if (effects & Attribute::EffectFadeIn) {
      QMapIterator<int, QVariant> it = dynamicAttribute()->properties();
      while (it.hasNext()) {
        it.next();
        if (attrib->hasProperty(it.key())) {
          attrib->setProperty(it.key(), mergeWith(attrib->property(it.key()), it.value(), m_sequence));
        } else {
          attrib->setProperty(it.key(), mergeWith(QVariant(), it.value(), m_sequence));
        }
      }

    } else {
      attrib->merge(*dynamicAttribute());
    }

  } else if (m_sequence > 200 && m_sequence <= 300) {
    if (effects & Attribute::EffectFadeOut) {
      QMapIterator<int, QVariant> it = dynamicAttribute()->properties();
      while (it.hasNext()) {
        it.next();
        if (attrib->hasProperty(it.key())) {
          attrib->setProperty(it.key(), mergeWith(attrib->property(it.key()), it.value(), 300 - m_sequence));
        } else {
          attrib->setProperty(it.key(), mergeWith(QVariant(), it.value(), 300 - m_sequence));
        }
      }

    } else {
      attrib->merge(*dynamicAttribute());
    }

  } else {
    attrib->merge(*dynamicAttribute());
  }
}

void KateDynamicAnimation::finish( )
{
  if (!(dynamicAttribute()->effects() & Attribute::EffectFadeOut))
    m_sequence = 300;

  else if (m_sequence < 100)
    // if the animation didn't make it through the intro, make the outro the same length
    m_sequence = 300 - m_sequence;
  else
    m_sequence = 200;

  m_timer->start();
}

QVariant KateDynamicAnimation::mergeWith( const QVariant & baseVariant, const QVariant & dynamicVariant, int percent ) const
{
  //Q_ASSERT(baseVariant.type() == dynamicVariant.type());

  double baseFactor = double(100 - percent) / 100;
  double addFactor = double(percent) / 100;

  switch (dynamicVariant.type()) {
    case QVariant::Pen: {
      QPen dynamic = qVariantValue<QPen>(dynamicVariant);

      QColor ret;

      if (baseVariant.type() == QVariant::Pen) {
        QColor base = qVariantValue<QPen>(baseVariant).color();

        int r1, g1, b1;
        base.getRgb(&r1, &g1, &b1);

        int r2, g2, b2;
        dynamic.color().getRgb(&r2, &g2, &b2);

        double r3, g3, b3;

        r3 = r1 * baseFactor + addFactor * r2;
        g3 = g1 * baseFactor + addFactor * g2;
        b3 = b1 * baseFactor + addFactor * b2;

        ret.setRgb((int)r3, (int)g3, (int)b3);

      } else {
        ret = dynamic.color();
        ret.setAlpha(int(255 * addFactor));
      }

      dynamic.setColor(ret);

      return dynamic;
    }

    case QVariant::Brush: {
      QBrush dynamic = qVariantValue<QBrush>(dynamicVariant);

      QColor ret;

      if (baseVariant.type() == QVariant::Brush) {
        QBrush base = qVariantValue<QBrush>(baseVariant);

        int r1, g1, b1;
        base.color().getRgb(&r1, &g1, &b1);

        int r2, g2, b2;
        dynamic.color().getRgb(&r2, &g2, &b2);

        double r3, g3, b3;

        r3 = r1 * baseFactor + addFactor * r2;
        g3 = g1 * baseFactor + addFactor * g2;
        b3 = b1 * baseFactor + addFactor * b2;

        ret.setRgb((int)r3, (int)g3, (int)b3);

      } else {
        ret = dynamic.color();
        ret.setAlpha(int(255 * addFactor));
      }

      dynamic.setColor(ret);

      return dynamic;
    }

    default:
      break;
  }

  return dynamicVariant;
}

#include "katedynamicanimation.moc"
