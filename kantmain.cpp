/***************************************************************************
                          kantmain.cpp  -  description
                             -------------------
    begin                : Wed Jan 3 2001
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

#include <kstddirs.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kstdaction.h>
#include <kaboutdata.h>
#include <kglobal.h>
#include <kglobalaccel.h>
#include <kglobalsettings.h>

#include "./app/kantapp.h"
#include <cstdlib>

static KCmdLineOptions options[] =
{
    { "+file(s)",          I18N_NOOP("Files to load"), 0 },
    { 0,0,0 }
};

int main( int argc, char **argv )
{
  KAboutData aboutData ("kant", I18N_NOOP("Kant"), "0.2",
	I18N_NOOP( "Kant - A MDI Texteditor for KDE" ), KAboutData::License_GPL,
	 "(c) 2000-2001 The Kant Authors");

  aboutData.addAuthor("Christoph Cullmann", "Project Manager and Core Developer", "crossfire@babylon2k.de", "http://www.babylon2k.de");
  aboutData.addAuthor("Michael Bartl", "Core Developer", "michael.bartl1@chello.at");
  aboutData.addAuthor("Phlip", "The Project Compiler", "phlip_cpp@my-deja.com");
  aboutData.addAuthor("Anders Lund", "Core Developer", "anders@alweb.dk", "http://www.alweb.dk");
  aboutData.addAuthor("Matt Newell", "Testing, ...", "newellm@proaxis.com");
  aboutData.addAuthor("Joseph Wenninger", "Core Developer", "jowenn@bigfoot.com");
  aboutData.addAuthor("Michael McCallum", "Core Developer", "gholam@xtra.co.nz");

  KCmdLineArgs::init (argc, argv, &aboutData);
  KCmdLineArgs::addCmdLineOptions (options);
  KantApp::addCmdLineOptions ();

  if (!KantApp::start())
    exit(0);

  KantApp app;
  return app.exec();
}
