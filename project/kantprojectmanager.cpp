/***************************************************************************
                          kantprojectmanager.cpp  -  description
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

#include "kantprojectmanager.h"
#include "kantprojectmanager.moc"

#include <iostream.h>
#include <qtextstream.h>
#include <klocale.h>
#include "kantprojectdialog.h"
#include "../piper/piper.cpp"  //  PCP I feel not a scrap of guilt...

KantProjectManager::KantProjectManager (KantDocManager *docManager, KantViewManager *viewManager, KStatusBar *statusBar) : QObject()
{
  this->docManager = docManager;
  this->viewManager = viewManager;
  this->statusBar = statusBar;
}

KantProjectManager::~KantProjectManager ()
{
}

/* 
   Open a "new Project" dialog, when pressed ok
   close all open windows (to have a clean workspace)
*/
void KantProjectManager::slotProjectNew()
{
	KantProjectDialog* kantprojectdialog = new KantProjectDialog();
	if (kantprojectdialog->exec())
	{
		/* Now create project here */
	}
}

/* Open a project */
// TODO: close all open files
// TODO: check if the filenames in the project are valid

void KantProjectManager::slotProjectOpen()
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
		viewManager->openURL(file_url);
	}

	projectFile = url;
}

/*
   All files opened at this point should be related to
   the project, because they are saved into the project file
*/
void KantProjectManager::slotProjectSave()
{
	// now generate a new XML file based on the project information
	QDomDocument xml("kant_project");
	QDomElement top = xml.createElement("kant_project");
	top.setAttribute("name", "project_name_here");
	top.setAttribute("version", "1");

	for (KantDocument* k = docManager->firstDoc(); k != 0; k = docManager->nextDoc())
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
void KantProjectManager::slotProjectSaveAs()
{
	// KURL of the project file to save
	KURL url = KFileDialog::getSaveURL(QString::null, i18n("Project File (*.prj)"), 0L, i18n("Save Project..."));

	if (!url.isEmpty())
	{
		projectFile = url;
		slotProjectSave();
	}
}

void KantProjectManager::slotProjectConfigure()
{
}

void KantProjectManager::slotProjectRun()
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


void KantProjectManager::slotProjectCompile()
{

  //  insert a line here to save all documents

  FILE *fp = NULL;
  assert (statusBar);


  /*  assume the user changed the file & wants to compile it...  */

  /*  tip: put 2>&1 in your script so we can see the "error" messages  */

  fp = popen ("./builder.sh", "r");

  if (fp)
    readAnyErrors(fp, *statusBar);
}
