/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Dominik Haumann (dhdev@gmx.de) (documentation)

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

#ifndef KDELIBS_KTEXTEDITOR_SESSIONCONFIGINTERFACE_H
#define KDELIBS_KTEXTEDITOR_SESSIONCONFIGINTERFACE_H

#include <ktexteditor/ktexteditor_export.h>

class KConfigGroup;

#include <QtCore/QObject>

namespace KTextEditor
{

/**
 * \brief Session config interface extension for the Document, View and Plugin.
 *
 * \ingroup kte_group_doc_extensions
 * \ingroup kte_group_view_extensions
 * \ingroup kte_group_plugin_extensions
 *
 * \section sessionconfig_intro Introduction
 *
 * The SessionConfigInterface is an extension for Documents, Views and Plugins
 * to add support for session-specific configuration settings.
 * readSessionConfig() is called whenever session-specific settings are to be
 * read from the given KConfig* and writeSessionConfig() whenever they are to
 * be written, for example when a session changed or was closed.
 *
 * \note A \e session does not have anything to do with an X-session under Unix.
 *       What is meant is rather a context, think of sessions in Kate or
 *       projects in KDevelop for example.
 *
 * \section sessionconfig_support Adding Session Support
 *
 * To add support for sessions a KTextEditor implementation has to derive the
 * Document and View class from SessionConfigInterface and reimplement
 * readSessionConfig() and writeSessionConfig().
 *
 * The same applies to a Plugin, read the detailed description for plugins.
 *
 * \section sessionconfig_access Accessing the SessionConfigInterface
 *
 * The SessionConfigInterface is supposed to be an extension interface for a
 * Document, a View or a Plugin, i.e. the Document/View/Plugin inherits the
 * interface \e provided that it implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // object is of type KTextEditor::Document* or View* or Plugin*
 * KTextEditor::SessionConfigInterface *iface =
 *     qobject_cast<KTextEditor::SessionConfigInterface*>( object );
 *
 * if( iface ) {
 *     // interface is supported
 *     // do stuff
 * }
 * \endcode
 *
 * \see KTextEditor::Document, KTextEditor::View, KTextEditor::Plugin
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KTEXTEDITOR_EXPORT SessionConfigInterface
{
  public:
    SessionConfigInterface();

    /**
     * Virtual destructor.
     */
    virtual ~SessionConfigInterface();

  //
  // SLOTS !!!
  //
  public:
    /**
     * Read session settings from the given \p config.
     *
     * That means for example
     *  - a Document should reload the file, restore all marks etc...
     *  - a View should scroll to the last position and restore the cursor
     *    position etc...
     *  - a Plugin should restore session specific settings
     *  - If no file is being loaded, because an empty new document is going to be displayed,
     *    this function should emit ReadOnlyPart::completed
     *
     * \param config read the session settings from this KConfigGroup
     * \see writeSessionConfig()
     */
    virtual void readSessionConfig (const KConfigGroup& config) = 0;

    /**
     * Write session settings to the \p config.
     * See readSessionConfig() for more details.
     *
     * \param config write the session settings to this KConfigGroup
     * \see readSessionConfig()
     */
    virtual void writeSessionConfig (KConfigGroup& config) = 0;

  private:
    class SessionConfigInterfacePrivate* const d;
};

}

Q_DECLARE_INTERFACE(KTextEditor::SessionConfigInterface, "org.kde.KTextEditor.SessionConfigInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
