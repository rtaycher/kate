/* This file is part of the KDE libraries
   Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>

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

#ifndef __ktexteditor_templateinterface_h__
#define __ktexteditor_templateinterface_h__

#include <qstring.h>
#include <qmap.h>
#include <qwidget.h>

#include <kdelibs_export.h>

namespace KTextEditor
{

class Document;

/**
 * This is an interface for inserting template strings with user editable
 * fields into a document.
 */
class KTEXTEDITOR_EXPORT TemplateInterface //should be named AbstractTemplateInterface, but for consistency with the other classes it is not (for the 3.x release series)
{
  friend class PrivateTemplateInterface;

  public:
    TemplateInterface();
    virtual ~TemplateInterface();

    uint templateInterfaceNumber () const;

  protected:
    void setTemplateInterfaceDCOPSuffix (const QCString &suffix);

  public:

    /**
     * Inserts an interactive ediable template text at line "line", column "col".
     * @p parentWindow is used if dialogs have to be shown
     * @return true if inserting the string succeeded
     *
     * Use insertTemplateText(numLines(), ...) to append text at end of document
     * Template  strings look like
     * "for( int ${index}=0;${index}<10;${index}++) { ${cursor} };"
     * This syntax is similiar to the one found in the Eclipse editor
     * There are certain common placeholders (variables), which get assigned a
     * default initialValue, If the second parameter does not a given value.
     * For all others the initial value is the name of the placeholder.
     *
     * Placeholder names may only consist of a-zA-Z0-9_
     * Common placeholders and values are
     *
     * - index: "i"
     * - loginname: The current users's loginname
     * - firstname: The current user's first name retrieved from kabc
     * - lastname: The current user's last name retrieved from kabc
     * - fullname: The current user's first and last name retrieved from kabc
     * - email: The current user's primary email adress  retrieved from kabc
     * - date: current date
     * - time: current time
     * - year: current year
     * - month: current month
     * - day: current day
     * - hostname: hostname of the computer
     * - cursor: at this position the cursor will be after editing of the
     *   template has finished, this has to be taken care of by the actual
     *   implementation. The placeholder gets a value of "|" assigned.
     *
     * If the editor supports some kind of smart indentation, the inserted code
     * should be layouted by the indenter.
     */
    bool insertTemplateText ( uint line, uint column, const QString &templateString, const QMap<QString,QString> &initialValues, QWidget *parentWindow=0);

protected:
    /**
     * You must implement this, it is called by insertTemplateText, after all
     * default values are inserted.
     */
    virtual bool insertTemplateTextImplementation ( uint line, uint column, const QString &templateString, const QMap<QString,QString> &initialValues, QWidget *parentWindow=0 )=0;

  /**
  * only for the interface itself - REAL PRIVATE
  */
  private:
    class PrivateTemplateInterface *d;
    static uint globalTemplateInterfaceNumber;
    uint myTemplateInterfaceNumber;
};

KTEXTEDITOR_EXPORT TemplateInterface *templateInterface (Document *doc);

}

#endif
