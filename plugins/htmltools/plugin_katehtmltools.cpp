/***************************************************************************
                          plugin_katehtmltools.cpp  -  description
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

#include "plugin_katehtmltools.h"
#include "plugin_katehtmltools.moc"

#include <qinputdialog.h>
#include <kaction.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <cassert>
#include <kdebug.h>
#include <qstring.h>

extern "C"
{
  void* init_libkatehtmltoolsplugin()
  {
    return new KatePluginFactory;
  }
}

KatePluginFactory::KatePluginFactory()
{
  s_instance = new KInstance( "kate" );
}

KatePluginFactory::~KatePluginFactory()
{
  delete s_instance;
}

QObject* KatePluginFactory::createObject( QObject* parent, const char* name, const char*, const QStringList & )
{
  return new PluginKateHtmlTools( parent, name );
}

KInstance* KatePluginFactory::s_instance = 0L;

PluginKateHtmlTools::PluginKateHtmlTools( QObject* parent, const char* name )
    : KatePluginIface ( parent, name )
{
}

PluginKateHtmlTools::~PluginKateHtmlTools()
{
}

KatePluginViewIface *PluginKateHtmlTools::createView ()
{
   KatePluginViewIface *view = new KatePluginViewIface ();

(void)  new KAction ( i18n("HT&ML Tag..."), "edit_HTML_tag", ALT + Key_Minus, this,
                                SLOT( slotEditHTMLtag() ), view->actionCollection(), "edit_HTML_tag" );

   view->setXML( "plugins/katehtmltools/ui.rc" );
   viewList.append (view);
   return view;
}

void PluginKateHtmlTools::slotEditHTMLtag()
//  PCP
{
  KateViewIface *kv=appIface->viewManagerIface()->getActiveView();
  if (!kv) return;

  QString text ( KatePrompt ( i18n("HTML Tag"),
                        i18n("Enter HTML tag contents. We will supply the <, > and closing tag"),
                        (QWidget *)appIface->viewManagerIface()->getActiveView())
                         );

  if ( !text.isEmpty () )
      slipInHTMLtag (*kv, text); // user entered something and pressed ok

}


QString PluginKateHtmlTools::KatePrompt
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


void PluginKateHtmlTools::slipInHTMLtag (KateViewIface & view, QString text)  //  PCP
{

  //  We must add a heavy elaborate HTML markup system. Not!

  QStringList list = QStringList::split (' ', text);
  QString marked (view.markedText ());
  int preDeleteLine = -1, preDeleteCol = -1;
  view.getCursorPosition (&preDeleteLine, &preDeleteCol);
  assert (preDeleteLine > -1);  assert (preDeleteCol > -1);


  if (marked.length() > 1)
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
