/***************************************************************************
                          plugin_kanthtmltools.cpp  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "plugin_kanthtmltools.h"

#include <qinputdialog.h>
#include <kaction.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <klocale.h>
#include "../../kwrite/kwview.h"
#include <cassert>  
#include <kdebug.h>
#include <qstring.h>

PluginKantHtmlTools::PluginKantHtmlTools( QObject* parent, const char* name )
    : Plugin( parent, name )
{
    // Instantiate all of your actions here.  These will appear in
    // Konqueror's menu and toolbars.

(void)  new KAction ( i18n("HT&ML Tag..."), "edit_HTML_tag", ALT + Key_Minus, this,
                                SLOT( slotEditHTMLtag() ), actionCollection(), "edit_HTML_tag" );

   myParent=(KantPluginIface *)parent;
}

PluginKantHtmlTools::~PluginKantHtmlTools()
{
}

void PluginKantHtmlTools::slotEditHTMLtag()
//  PCP
{
 
  if (!myParent)  return;
  KWrite *kv=myParent->getActiveView();
  if (!kv) return;
 
  QString text ( KantPrompt ( i18n("HTML Tag"),
                        i18n("Enter HTML tag contents. We will supply the <, > and closing tag"),
                        myParent
                        ) );
 
  if ( !text.isEmpty () )
      slipInHTMLtag (*kv, text); // user entered something and pressed ok
 
}


QString PluginKantHtmlTools::KantPrompt
        (
        QString strTitle,
        QString strPrompt,
        QWidget * that
        )
{

  bool ok (false);

  //  TODO: Make this a "memory edit" field with a combo box
  //  containing prior entries

  QString text ( QInputDialog::getText
			(
			strTitle,
			strPrompt,
			QString::null,
			&ok,
			that
			) );

  if (!ok) text = "";
  return text;

}


void PluginKantHtmlTools::slipInHTMLtag (KWrite & view, QString text)  //  PCP
{

  //  We must add a <em>heavy</em> elaborate HTML markup system. Not!

  QStringList list = QStringList::split (' ', text);
  QString marked (view.markedText ());
  int preDeleteLine = -1, preDeleteCol = -1;
  view.getCursorPosition (&preDeleteLine, &preDeleteCol);
  assert (preDeleteLine > -1);  assert (preDeleteCol > -1);

  //  shoot me for strlen() but it worked better than .length() for some reason...
  // lukas: BANG! strlen() fucks up non latin1 characters :-)

  if (marked.length() > 0)
    view.keyDelete ();
  int line = -1, col = -1;
  view.getCursorPosition (&line, &col);
  assert (line > -1);  assert (col > -1);
  QString pre ("<" + text + ">");
  QString post;
  if (list.count () > 0)  post = "</" + list[0] + ">";
  view.insertText (pre + marked + post);

  //  all this muck to leave the cursor exactly where the user
  //  put it...

  //  Someday we will can all this (unless if it already
  //  is canned and I didn't find it...)

  //  The second part of the if disrespects the display bugs
  //  when we try to reselect. TODO: fix those bugs, and we can
  //  un-break this if...

  if (preDeleteLine == line && -1 == marked.find ('\n'))
    if (preDeleteLine == line && preDeleteCol == col)
        {
        view.setCursorPosition (line, col + pre.length () + marked.length () - 1);

        for (int x (marked.length());  x--;)
                view.shiftCursorLeft ();
        }
    else
        {
        view.setCursorPosition (line, col += pre.length ());

        for (int x (marked.length());  x--;)
                view.shiftCursorRight ();
        }

}


kanthtmltoolsFactory::kanthtmltoolsFactory()
{
}

kanthtmltoolsFactory::~kanthtmltoolsFactory()
{
  delete s_instance;
  s_instance = 0;
}

QObject* kanthtmltoolsFactory::createObject( QObject* parent, const char* name, const char*, const QStringList & )
{
  return new PluginKantHtmlTools( parent, name );
}

KInstance *kanthtmltoolsFactory::instance()
{
  if ( !s_instance )
    s_instance = new KInstance( "htmltools" );
  return s_instance;
}

extern "C"
{
  void* init_libkanthtmltoolsplugin()
  {
    return new kanthtmltoolsFactory;
  }
}

KInstance* kanthtmltoolsFactory::s_instance = 0L;

#include <plugin_kanthtmltools.moc>
