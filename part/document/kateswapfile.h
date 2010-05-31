/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATE_SWAPFILE_H
#define KATE_SWAPFILE_H

#include <QtCore/QObject>

#include "katepartprivate_export.h"
#include "katetextbuffer.h"
#include "katebuffer.h"
#include "katedocument.h"

class QDataStream;

namespace Kate {

/**
 * Class for tracking editing actions.
 * In case Kate crashes, this can be used to replay all edit actions to
 * recover the lost data.
 */
class KATEPART_TESTS_EXPORT SwapFile : public QObject
{
  Q_OBJECT

  public:
    explicit SwapFile(KateDocument* document);

    void setTrackingEnabled(bool enabled);
    bool isTrackingEnabled() const;
  private:
    KateDocument *m_document;
    bool m_trackingEnabled;

  protected Q_SLOTS:
    void fileSaved(const QString& filename);
    void fileLoaded(const QString &filename);

    void startEditing ();
    void finishEditing ();

    void wrapLine (const KTextEditor::Cursor &position);
    void unwrapLine (int line);
    void insertText (const KTextEditor::Cursor &position, const QString &text);
    void removeText (const KTextEditor::Range &range);

  private:
    QDataStream *m_stream;
};

}

#endif // KATE_SWAPFILE_H
