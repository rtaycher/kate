/* This file is part of the KDE libraries
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_EDITOR_H
#define KDELIBS_KTEXTEDITOR_EDITOR_H

// our main baseclass of the KTextEditor::Editor
#include <QObject>

// pixmap of config page
#include <QPixmap>

// icon size enum
#include <kicontheme.h>

class KAboutData;
class KConfig;

namespace KTextEditor
{

class Document;
class ConfigPage;

/**
 * Accessor interface for Editor part.
 *
 * <b>Introduction</b>\n
 * The Editor part can be accessed via the KTextEditor::Factory and provides
 * general information and configuration methods for the Editor
 * implementation, for example KAboutData by using aboutData().
 *
 * The Editor implementation has a list of all opened documents. Get this
 * list with documents(). To create a new Document call createDocument(). The
 * signal documentCreated() is emitted whenever the Editor created a new
 * document.
 *
 * <b>Editor Configuration</b>\n
 *
 * If the Editor implementation supports a config dialog
 * configDialogSupported() returns @e true, then the config dialog can be
 * shown with configDialog(). Additionally config pages can be embedded into
 * the application's config dialog. configPages() returns the number of
 * config pages the Editor implementation provides and configPage() returns
 * the requested page. Further a config page has a short descriptive name,
 * get it with configPageName(). You can get more detailed name by using
 * configPageFullName. Also evary config page has a pixmap, get it with
 * configPagePixmap. Use the config dialog only if there are no config pages.
 * The configuration can be saved with readConfig() and writeConfig().
 *
 * <b>Implementation Notes</b>\n
 *
 * Usually only one instance of the Editor exists. The Kate Part internally
 * implementation uses a static accessor to make sure that only Kate Part
 * Editor object exists. So several factories still use the same Editor.
 *
 * @see KTextEditor::Factory, KTextEditor::Document
 * @author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT Editor : public QObject
{
  Q_OBJECT

  public:
    /**
     * Constructor.
     *
     * Create the Editor object with @p parent.
     * @param parent parent object
     */
    Editor ( QObject *parent );

    /**
     * Virtual destructor.
     */
    virtual ~Editor ();

  /*
   * Methods to create and manage the documents.
   */
  public:
    /**
     * Create a new document object with @p parent.
     * @param parent parent object
     * @return new KTextEditor::Document object
     * @see documents()
     */
    virtual Document *createDocument ( QObject *parent ) = 0;

    /**
     * Get a list of all documents of this editor.
     * @return list of all existing documents
     * @see createDocument()
     */
    virtual const QList<Document*> &documents () = 0;

  /*
   * General Information about this editor.
   */
  public:
    /**
     * Get the about data of this Editor part.
     * @return about data
     */
    virtual const KAboutData *aboutData () const = 0;

  /*
   * Configuration management.
   */
  public:
    /**
     * Read editor configuration from its standard config.
     * @see writeConfig()
     */
    virtual void readConfig () = 0;

    /**
     * Write editor configuration to its standard config.
     * @see readConfig()
     */
    virtual void writeConfig () = 0;

    /**
     * Read editor configuration from KConfig @p config.
     * @param config config object
     * @see writeConfig()
     */
    virtual void readConfig (KConfig *config) = 0;

    /**
     * Write editor configuration to KConfig @p config.
     * @param config config object
     * @see readConfig()
     */
    virtual void writeConfig (KConfig *config) = 0;

    /**
     * Check, whether this editor has a configuration dialog.
     * @return @e true, if the editor has a configuration dialog,
     *         otherwise @e false
     * @see configDialog()
     */
    virtual bool configDialogSupported () const = 0;

    /**
     * Show the editor's config dialog, changes will be applied to the
     * editor, but not saved anywhere automagically, call @p writeConfig()
     * to save them.
     * @param parent parent widget
     * @see configDialogSupported()
     */
    virtual void configDialog (QWidget *parent) = 0;

    /**
     * Get the number of available config pages.
     * If the editor returns a number < 1, it does not support config pages
     * and the embedding application should use configDialog() instead.
     * @return number of config pages
     * @see configPage(), configDialog()
     */
    virtual int configPages () const = 0;

    /**
     * Get the config page with the @p number, config pages from 0 to
     * configPages()-1 are available if configPages() > 0.
     * @param number index of config page
     * @param parent parent widget for config page
     * @return created config page or NULL, if the number is out of bounds
     * @see configPages()
     */
    virtual ConfigPage *configPage (int number, QWidget *parent) = 0;

    /**
     * Get a readable name for the config page @p number. The name should be
     * translated.
     * @param number index of config page
     * @return name of given page index
	 * @see configPageFullName(), configPagePixmap()
     */
    virtual QString configPageName (int number) const = 0;

    /**
     * Get a readable full name for the config page @e number. The name
     * should be translated.
     *
     * Example: If the name is "Filetypes", the full name could be
     * "Filetype Specific Settings". For "Shortcuts" the full name would be
     * something like "Shortcut Configuration".
     * @param number index of config page
     * @return full name of given page index
	 * @see configPageName(), configPagePixmap()
     */
    virtual QString configPageFullName (int number) const = 0;

    /**
     * Get a pixmap with @p size for the config page @p number.
     * @param number index of config page
     * @param size size of pixmap
     * @return pixmap for the given page index
     * @see configPageName(), configPageFullName()
     */
    virtual QPixmap configPagePixmap (int number, int size = KIcon::SizeSmall) const = 0;

  signals:
    /**
     * The @p editor emits this signal whenever a @p document was successfully created.
     * @param editor editor which created the new document
     * @param document the newly created document instance
     * @see createDocument()
     */
    void documentCreated (KTextEditor::Editor *editor, KTextEditor::Document *document);
};

KTEXTEDITOR_EXPORT Editor *editor ( const char *libname );

}

#endif
