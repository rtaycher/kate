/* This file is part of the KDE project
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KATE_DOCMANAGER_INCLUDE_
#define _KATE_DOCMANAGER_INCLUDE_

#include <kdebase_export.h>

#include <qobject.h>
#include <kurl.h>

namespace KTextEditor { class Document; }

namespace Kate
{
/**
 * \brief Interface for the document manager.
 *
 * This interface provides access to Kate's document manager. The document
 * manager manages all documents. Use document() get a given document,
 * activeDocument() to retrieve the active document. Check with isOpen()
 * whether an URL is opened and use findDocument() to get it. To get the
 * number of managed documents use documents().
 *
 * Open new documents with openURL() and close a document with closeDocument()
 * or closeAllDocuments(). Several signals are provided, documentChanged() is
 * emitted whenever the document's content changed, documentCreated() when a
 * new document was created and documentDeleted() when a document was closed.
 * 
 * To access the document manager use the global accessor function
 * documentManager() or Application::documentManager(). You should never have
 * to create an instance of this class yourself.
 *
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KATEINTERFACES_EXPORT DocumentManager : public QObject
{
  friend class PrivateDocumentManager;

  Q_OBJECT

  public:
    /**
     * Construtor.
     *
     * The constructor is internally used by the Kate application, so it is
     * of no interest for plugin developers. Plugin developers should use the
     * global accessor pluginManager() instead.
     *
     * \param documentManager internal usage
     *
     * \internal
     */
    DocumentManager ( void *documentManager  );
    /**
     * Virtual destructor.
     */
    virtual ~DocumentManager ();

  public:
    /**
     * Get the document with the index @p n.
     * \param n document index
     * \return a pointer to the document indexed by @p n in the managers
     *         internal list
     * \see activeDocument(), documents()
     */
    class KTextEditor::Document *document (uint n = 0);

    /**
     * Get the currently active document.
     * \return a pointer to the currently active document or NULL if no
     *         document is opened.
     */
    class KTextEditor::Document *activeDocument ();

    /**
     * Get the document with the URL \p url.
     * \param url the document's URL
     * \return the document with the given \p url or NULL, if no such document
     *         is in the document manager's internal list.
     */
    KTextEditor::Document *findDocument (const KUrl &url);

    /**
     * Check whether a document with given \p url is opened.
     * \param url the document's url
     * \return \e true if the document \p url is opened, otherwise \e false
     */
    bool isOpen (const KUrl &url);

    /**
     * Get the number of documents the document manager manages.
     * \return the number of documents managed by this manager
     */
    uint documents ();

    /**
     * Open the document \p url with the given \p encoding.
     * \param url the document's url
     * \param encoding the preferred encoding. If encoding is QString() the
     *        encoding will be guessed or the default encoding will be used.
     * \return a pointer to the created document
     */
    KTextEditor::Document *openURL(const KUrl&url,const QString &encoding=QString());

    /**
     * Close the given \p document.
     * \param document the document to be closed
     * \return \e true on success, otherwise \e false
     */
    bool closeDocument(KTextEditor::Document *document);

    /**
     * Close the document identified by index \p n.
     * \param n the document's index
     * \return \e true on success, otherwise \e false
     */
    bool closeDocument(uint n = 0);

    /**
     * Close all documents.
     * \return \e true on success, otherwise \e false
     */
    bool closeAllDocuments();


  //
  // SIGNALS !!!
  //
#ifndef Q_MOC_RUN
  #undef signals
  #define signals public
#endif
  signals:
#ifndef Q_MOC_RUN
  #undef signals
  #define signals protected
#endif

    /**
     * This signal is emitted whenever the current document's content was changed.
     * Note, there does not need to be an active document.
     */
    void documentChanged ();
    
    /**
     * This signal is emitted when the \p document was created.
     */
    void documentCreated (KTextEditor::Document *document);
    
    /**
     * This signal is emitted when the \p document was closed.
     */
    void documentDeleted (KTextEditor::Document *document);

  private:
    class PrivateDocumentManager *d;
};

/**
 * Global accessor to the document manager object.
 * \return document manager object
 */
DocumentManager *documentManager ();

}

#endif
