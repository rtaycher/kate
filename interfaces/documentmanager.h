/***************************************************************************
                          docmanager.h -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
 ***************************************************************************/

#ifndef _KATE_DOCMANAGER_INCLUDE_
#define _KATE_DOCMANAGER_INCLUDE_

#include <qobject.h>
#include <kurl.h>

namespace Kate
{
/** This interface provides access to the Kate Document Manager.
*/
class DocumentManager : public QObject
{
  friend class PrivateDocumentManager;

  Q_OBJECT   
  
  public:
    DocumentManager ( void *documentManager  );
    ~DocumentManager ();

  public:
    /** Returns a pointer to the document indexed by n in the managers internal list.
    */
    class Document *document (uint n = 0);
    /** Returns a pointer to the currently active document or NULL if no document is open.
    */
    class Document *activeDocument ();
    /** Returns a pointer to the document with the given ID or NULL if no such document exists.
    */
    class Document *documentWithID (uint id);

    /** Returns the ID of the document located at url if such a document is known by the manager.
     */
    int findDocument (const KURL &url);
    /** Returns true if the document located at url is open, otherwise false.
     */
    bool isOpen (const KURL &url);

    /** returns the number of documents managed by this manager.
    */
    uint documents ();       

    /** open a document and return a pointer to the document, if you specify a pointer != 0 to the id parameter
     * you will get the document id returned too
     */    
    class Document *openURL(const KURL&url,const QString &encoding=QString::null,uint *id =0);
    /** close a document by pointer
     */
    bool closeDocument(class Document *document);
    /** close a document identified by the index
     */
    bool closeDocument(uint n = 0); 
    /** close a document identified by the ID
     */
    bool closeDocumentWithID(uint id);
    /** close all documents
     */
    bool closeAllDocuments();

  signals:
    void documentChanged ();
    
  public:
    void emitDocumentChanged ();
    
  private:
    class PrivateDocumentManager *d;
};

};

#endif
