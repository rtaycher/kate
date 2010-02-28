/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#ifndef KATE_TEXTLOADER_H
#define KATE_TEXTLOADER_H

#include <QtCore/QString>
#include <QtCore/QTextCodec>
#include <QtCore/QFile>

// on the fly compression
#include <kfilterdev.h>
#include <kmimetype.h>

namespace Kate {

/**
 * loader block size, load 256 kb at once per default
 * if file size is smaller, fall back to file size
 * must be a multiple of 2
 */
static const qint64 KATE_FILE_LOADER_BS  = 256 * 1024;

/**
 * File Loader, will handle reading of files + detecting encoding
 */
class FileLoader
{
  public:
    /**
     * End of line marker types
     */
    enum EndOfLineType {
        eolUnknown
      , eolUnix
      , eolDos
      , eolMac
    };

    /**
     * Construct file loader for given file and codec.
     * @param filename file to open
     * @param codec codec to use
     */
    FileLoader (const QString &filename, QTextCodec *codec)
      : m_codec(codec)
      , m_eof (false) // default to not eof
      , m_lastWasEndOfLine (true) // at start of file, we had a virtual newline
      , m_lastWasR (false) // we have not found a \r as last char
      , m_position (0)
      , m_lastLineStart (0)
      , m_eol (eolUnknown) // no eol type detected atm
      , m_buffer (KATE_FILE_LOADER_BS, 0)
    {
      // try to get mimetype for on the fly decompression, don't rely on filename!
      QFile testMime (filename);
      if (testMime.open (QIODevice::ReadOnly))
        m_mimeType = KMimeType::findByContent (&testMime)->name ();
      else
        m_mimeType = KMimeType::findByPath (filename, 0, false)->name ();

      // construct filter device
      m_file = KFilterDev::deviceForFile (filename, m_mimeType, false);
    }

    /**
     * Destructor
     */
    ~FileLoader ()
    {
      delete m_file;
    }

    /**
     * open file
     * @return success
     */
    bool open ()
    {
      return m_file->open (QIODevice::ReadOnly);
    }

    /**
     * end of file reached?
     * @return end of file reached
     */
    bool eof () const { return m_eof && !m_lastWasEndOfLine && (m_lastLineStart == m_text.length()); }

    /**
     * Detected end of line mode for this file.
     * Detected during reading, is valid after complete file is read.
     * @return eol mode of this file
     */
    EndOfLineType eol () const { return m_eol; }

    /**
     * mime type used to create filter dev
     * @return mime-type of filter device
     */
    const QString &mimeTypeForFilterDev () const { return m_mimeType; }

    /**
     * internal unicode data array
     * @return internal unicode data
     */
    const QChar *unicode () const { return m_text.unicode(); }

    /**
     * read a line, return length + offset in unicode data
     * @param offset offset into internal unicode data for read line
     * @param length lenght of read line
     */
    void readLine (int &offset, int &length)
    {
      length = 0;
      offset = 0;

      static const QLatin1Char cr(QLatin1Char('\r'));
      static const QLatin1Char lf(QLatin1Char('\n'));

      while (m_position <= m_text.length())
      {
          if (m_position == m_text.length())
        {
          // try to load more text if something is around
          if (!m_eof)
          {
            int c = m_file->read (m_buffer.data(), m_buffer.size());

            // kill the old lines...
            m_text.remove (0, m_lastLineStart);

            // if any text is there, append it....
            if (c > 0)
            {
              QString unicode = m_codec->toUnicode (m_buffer.constData(), c);
              m_text.append (unicode);
            }

            // is file completely read ?
            m_eof = (c == -1) || (c == 0);

            // recalc current pos and last pos
            m_position -= m_lastLineStart;
            m_lastLineStart = 0;
          }

          // oh oh, end of file, escape !
          if (m_eof && (m_position == m_text.length()))
          {
            m_lastWasEndOfLine = false;

            // line data
            offset = m_lastLineStart;
            length = m_position-m_lastLineStart;

            m_lastLineStart = m_position;

            return;
          }
        }

        if (m_text.at(m_position) == lf)
        {
          m_lastWasEndOfLine = true;

          if (m_lastWasR)
          {
            m_lastLineStart++;
            m_lastWasR = false;
            m_eol = eolDos;
          }
          else
          {
            // line data
            offset = m_lastLineStart;
            length = m_position-m_lastLineStart;

            m_lastLineStart = m_position+1;
            m_position++;

            // only win, if not dos!
            if (m_eol != eolDos)
              m_eol = eolUnix;

            return;
          }
        }
        else if (m_text.at(m_position) == cr)
        {
          m_lastWasEndOfLine = true;
          m_lastWasR = true;

          // line data
          offset = m_lastLineStart;
          length = m_position-m_lastLineStart;

          m_lastLineStart = m_position+1;
          m_position++;

          // should only win of first time!
          if (m_eol == eolUnknown)
            m_eol = eolMac;

          return;
        }
        else
        {
          m_lastWasEndOfLine = false;
          m_lastWasR = false;
        }

        m_position++;
      }
    }

  private:
    QTextCodec *m_codec;
    bool m_eof;
    bool m_lastWasEndOfLine;
    bool m_lastWasR;
    int m_position;
    int m_lastLineStart;
    EndOfLineType m_eol;
    QString m_mimeType;
    QIODevice *m_file;
    QByteArray m_buffer;
    QString m_text;
};

}

#endif
