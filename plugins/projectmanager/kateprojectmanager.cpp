/***************************************************************************
                          kateprojectmanager.cpp  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph "Crossfire" Cullmann
    email                : crossfire@babylon2k.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kateprojectmanager.h"
#include "kateprojectmanager.moc"

#include "kateprojectdialog.h"

#include <kxmlgui.h>
#include <kfiledialog.h>
#include <iostream.h>
#include <qtextstream.h>
#include <kstatusbar.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <kaction.h>
#include "./piper/piper.h"

extern "C"
{
  void* init_libkateprojectmanagerplugin()
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
  return new KateProjectManager( parent, name );
}

KInstance* KatePluginFactory::s_instance = 0L;

KateProjectManager::KateProjectManager (QObject* parent, const char* name) : KatePluginIface(parent, name)
{
}

KateProjectManager::~KateProjectManager ()
{
}

KatePluginViewIface *KateProjectManager::createView ()
{
   KatePluginViewIface *view = (KatePluginViewIface *) new KateProjectManagerView (this);

   viewList.append (view);
   return view;
}

/*
   Open a "new Project" dialog, when pressed ok
   close all open windows (to have a clean workspace)
*/
void KateProjectManager::slotProjectNew()
{
	KateProjectDialog* kateprojectdialog = new KateProjectDialog();
	if (kateprojectdialog->exec())
	{
		/* Now create project here */
	}
}

/* Open a project */
// TODO: close all open files
// TODO: check if the filenames in the project are valid

void KateProjectManager::slotProjectOpen()
{
	KURL url = KFileDialog::getOpenURL(QString::null, QString::null, 0L, i18n("Load Project..."));
	if ( url.isMalformed() )
		return;

	// create a QDomDocument to parse the XML file
	QDomDocument doc("projectFile");

	// reference the chosen file
	QFile f(url.path());
	f.open(IO_ReadOnly);

	// set the content of the file to the QDomDocument
	doc.setContent(&f);

	// close file
	f.close();

	QDomElement docElement = doc.documentElement();
	for (QDomNode n = docElement.firstChild(); !n.isNull(); n = n.nextSibling())
	{
		QDomElement e = n.toElement();
		KURL file_url = e.attribute("path");
		appIface->viewManagerIface()->openURL(file_url);
	}

	projectFile = url;
}

/*
   All files opened at this point should be related to
   the project, because they are saved into the project file
*/
void KateProjectManager::slotProjectSave()
{
	// now generate a new XML file based on the project information
	QDomDocument xml("kate_project");
	QDomElement top = xml.createElement("kate_project");
	top.setAttribute("name", "project_name_here");
	top.setAttribute("version", "1");

	for (KateDocumentIface* k = appIface->docManagerIface()->getFirstDoc(); k != 0; k = appIface->docManagerIface()->getNextDoc())
	{
		QDomElement file = xml.createElement("file");
		file.setAttribute("path", k->url().path());
		top.appendChild(file);
	}

	// append the whole tree
	xml.appendChild(top);

	QFile f(projectFile.path());	// make a file reference
	f.open(IO_WriteOnly);		// open it for writing
	QTextStream t(&f);		// access it with a textstream
	t << xml.toString();		// pipe xml-doc into textstream
	f.close();			// finished by closing file
}

/* get a file for this project then call save() */
void KateProjectManager::slotProjectSaveAs()
{
	// KURL of the project file to save
	KURL url = KFileDialog::getSaveURL(QString::null, i18n("Project File (*.prj)"), 0L, i18n("Save Project..."));

	if (!url.isEmpty())
	{
		projectFile = url;
		slotProjectSave();
	}
}

void KateProjectManager::slotProjectConfigure()
{
}

void KateProjectManager::slotProjectRun()
{
}

static Piper s_aPiper;


static void readAnyErrors(FILE * fp, KStatusBar & bar)
{

	string strFileName;
	string strMessage ("Internal error reading message from compiler output");
	size_t nLine;

	s_aPiper.Reset();

	while (!feof(fp))
		{
		int c = fgetc (fp);

		if (c != EOF)
			{
			s_aPiper.StoreChar(c); /* sniff for diagnostics */
			putchar (c);
			if (c == '\n')	fflush (stdout);
			}
		else
			break;
		}
	fflush (stdout);
	pclose (fp);

	if (s_aPiper.GetNextError(strFileName, &nLine, strMessage))
		{

		//  TODO:  navigate to the document with the error...

		//  TODO:  go to the line with the error

                 //  TODO:  make this not block...

		//  TODO:  Get rid of this annoying error message & put something in the status bar...
		bar.message(strMessage.c_str());

		}

}


void KateProjectManager::slotProjectCompile()
{

  //  insert a line here to save all documents

  FILE *fp = NULL;
  assert (appIface->statusBar());


  /*  assume the user changed the file & wants to compile it...  */

  /*  tip: put 2>&1 in your script so we can see the "error" messages  */

  fp = popen ("./builder.sh", "r");

  if (fp)
    readAnyErrors(fp, *appIface->statusBar());
}

KateProjectManagerView::KateProjectManagerView(QObject *parent) : KatePluginViewIface (parent)
{
  setXML( "plugins/kateprojectmanager/ui.rc" );

 KActionMenu* pm_project  = new KActionMenu(i18n("&Project"), actionCollection(), "project");
 connect(pm_project->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(projectMenuAboutToShow()));

  // KActions for Project
   projectNew = new KAction(i18n("&New"), 0, (KateProjectManager*)pluginIface,
         SLOT(slotProjectNew()), actionCollection(),"project_new");
  projectOpen = new KAction(i18n("&Open"), 0, (KateProjectManager*)pluginIface,
         SLOT(slotProjectOpen()), actionCollection(),"project_open");
  projectSave = new KAction(i18n("&Save"), 0, (KateProjectManager*)pluginIface,
         SLOT(slotProjectSave()), actionCollection(),"project_save");
  projectSaveAs = new KAction(i18n("&SaveAs"), 0, (KateProjectManager*)pluginIface,
         SLOT(slotProjectSaveAs()), actionCollection(),"project_save_as");
  projectConfigure = new KAction(i18n("&Configure"), 0, (KateProjectManager*)pluginIface,
         SLOT(slotProjectConfigure()), actionCollection(),"project_configure");
  projectCompile = new KAction(i18n("&Compile"), Key_F5, (KateProjectManager*)pluginIface,
         SLOT(slotProjectCompile()), actionCollection(),"project_compile");
  projectRun = new KAction(i18n("&Run"), 0, (KateProjectManager*)pluginIface,
         SLOT(slotProjectRun()), actionCollection(),"project_run");
}

KateProjectManagerView::~KateProjectManagerView ()
{

}

void KateProjectManagerView::projectMenuAboutToShow()
{
  projectConfigure->setEnabled(false);
  projectRun->setEnabled(false);

  if (((KateProjectManager*)pluginIface)->projectFile.isEmpty())
    projectSave->setEnabled(false);
  else
    projectSave->setEnabled(true);

  if (pluginIface->appIface->docManagerIface()->getDocCount () == 0)
   projectSaveAs->setEnabled(false);
  else
    projectSaveAs->setEnabled(true);
}

