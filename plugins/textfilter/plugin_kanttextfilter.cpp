/***************************************************************************
                          plugin_kanttextfilter.cpp  -  description
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

#include "plugin_kanttextfilter.h"
#include "plugin_kanttextfilter.moc"

#include <qinputdialog.h>
#include <kaction.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <cassert>
#include <kdebug.h>
#include <qstring.h>

#define POP_(x) kdDebug(13000) << #x " = " << flush << x << endl

extern "C"
{
  void* init_libkanttextfilterplugin()
  {
    return new KantPluginFactory;
  }
}

KantPluginFactory::KantPluginFactory()
{
  s_instance = new KInstance( "kant" );
}

KantPluginFactory::~KantPluginFactory()
{
  delete s_instance;
}

QObject* KantPluginFactory::createObject( QObject* parent, const char* name, const char*, const QStringList & )
{
  return new PluginKantTextFilter( parent, name );
}

KInstance* KantPluginFactory::s_instance = 0L;


PluginKantTextFilter::PluginKantTextFilter( QObject* parent, const char* name )
    : KantPluginIface ( parent, name ),
  m_pFilterShellProcess (NULL)
{
}

PluginKantTextFilter::~PluginKantTextFilter()
{
  delete m_pFilterShellProcess;
}

KantPluginViewIface *PluginKantTextFilter::createView ()
{
   KantPluginViewIface *view = new KantPluginViewIface ();

  (void)  new KAction ( i18n("Fi&lter Text..."), "edit_filter", CTRL + Key_Backslash, this,
  SLOT( slotEditFilter() ), view->actionCollection(), "edit_filter" );

   view->setXML( "plugins/kanttextfilter/ui.rc" );
   viewList.append (view);
   return view;
}

        void
splitString (QString q, char c, QStringList &list)  //  PCP
{

// screw the OnceAndOnlyOnce Principle!

  int pos;
  QString item;

  while ( (pos = q.find(c)) >= 0)
    {
      item = q.left(pos);
      list.append(item);
      q.remove(0,pos+1);
    }
  list.append(q);
}


        static void  //  PCP
slipInNewText (KantViewIface & view, QString pre, QString marked, QString post, bool reselect)
{

  int preDeleteLine = -1, preDeleteCol = -1;
  view.getCursorPosition (&preDeleteLine, &preDeleteCol);
  assert (preDeleteLine > -1);  assert (preDeleteCol > -1);

  //  shoot me for strlen() but it worked better than .length() for some reason...
  // lukas: BANG! strlen() fucks up non latin1 characters :-)

//  POP_(marked.latin1 ());

  if (marked.length() > 0)
    view.keyDelete ();
  int line = -1, col = -1;
  view.getCursorPosition (&line, &col);
  assert (line > -1);  assert (col > -1);
  view.insertText (pre + marked + post);

  //  all this muck to leave the cursor exactly where the user
  //  put it...

  //  Someday we will can all this (unless if it already
  //  is canned and I didn't find it...)

  //  The second part of the if disrespects the display bugs
  //  when we try to reselect. TODO: fix those bugs, and we can
  //  un-break this if...

  //  TODO: fix OnceAndOnlyOnce between this module and plugin_kanthtmltools.cpp

  if (reselect && preDeleteLine == line && -1 == marked.find ('\n'))
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


        static QString  //  PCP
KantPrompt
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

  if (!ok)  text = "";
  return text;

}


	void
PluginKantTextFilter::slotFilterReceivedStdout (KProcess * pProcess, char * got, int len)
{

	assert (pProcess == m_pFilterShellProcess);

	if (got && len)
		{

  //  TODO: got a better idea?

  		while (len--)  m_strFilterOutput += *got++;
//		POP_(m_strFilterOutput);
		}

}


	void
PluginKantTextFilter::slotFilterReceivedStderr (KProcess * pProcess, char * got, int len)
	{
	slotFilterReceivedStdout (pProcess, got, len);
	}


	void
PluginKantTextFilter::slotFilterProcessExited (KProcess * pProcess)
{

	assert (pProcess == m_pFilterShellProcess);
	KantViewIface * kv (appIface->viewManagerIface()->getActiveView());
	if (!kv) return;
	QString marked (kv -> markedText ());
	if (marked.length() > 0)
          kv -> keyDelete ();
	kv -> insertText (m_strFilterOutput);
//	slipInNewText (*kv, "", m_strFilterOutput, "", false);
	m_strFilterOutput = "";

}


        static void  //  PCP
slipInFilter (KShellProcess & shell, KantViewIface & view, QString command)
{

  QString marked (view.markedText ());

//  POP_(command.latin1 ());
  shell.clearArguments ();
  shell << command.local8Bit ();

  shell.start (KProcess::NotifyOnExit, KProcess::All);

  shell.writeStdin (marked.local8Bit (), marked.length ());

  //  TODO: Put up a modal dialog to defend the text from further
  //  keystrokes while the command is out. With a cancel button...

}


	void
PluginKantTextFilter::slotFilterCloseStdin (KProcess * pProcess)
	{
	assert (pProcess == m_pFilterShellProcess);
	pProcess -> closeStdin ();
	}


                 void
PluginKantTextFilter::slotEditFilter ()  //  PCP
{
  KantViewIface * kv (appIface->viewManagerIface()->getActiveView());
  if (!kv) return;

  QString text ( KantPrompt ( i18n("Filter"),
                        i18n("Enter command to pipe selected text thru"),
                        (QWidget*)  appIface->viewManagerIface()->getActiveView()
                        ) );

  if ( !text.isEmpty () )
      {
      m_strFilterOutput = "";

      if (!m_pFilterShellProcess)
      	{
      	m_pFilterShellProcess = new KShellProcess;

	connect ( m_pFilterShellProcess, SIGNAL(wroteStdin(KProcess *)),
			   this, SLOT(slotFilterCloseStdin (KProcess *)));

	connect ( m_pFilterShellProcess, SIGNAL(receivedStdout(KProcess*,char*,int)),
	  	       this, SLOT(slotFilterReceivedStdout(KProcess*,char*,int)) );

	connect ( m_pFilterShellProcess, SIGNAL(receivedStderr(KProcess*,char*,int)),
	 	       this, SLOT(slotFilterReceivedStderr(KProcess*,char*,int)) );

	connect ( m_pFilterShellProcess, SIGNAL(processExited(KProcess*)),
		       this, SLOT(slotFilterProcessExited(KProcess*) ) ) ;
      	}

      slipInFilter (*m_pFilterShellProcess, *kv, text);
      }
}
