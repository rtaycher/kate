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
#include <kdebug.h>

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
	I18N_NOOP( "Kant - get an edge in editing" ), KAboutData::License_GPL,
	 "(c) 2000-2001 The Kant Authors", "http://devel-home.kde.org/~kant");

  aboutData.addAuthor("Christoph Cullmann", I18N_NOOP("Project Manager and Core Developer"), "crossfire@babylon2k.de", "http://www.babylon2k.de");
  aboutData.addAuthor("Michael Bartl", I18N_NOOP("Core Developer"), "michael.bartl1@chello.at");
  aboutData.addAuthor("Phlip", I18N_NOOP("The Project Compiler"), "phlip_cpp@my-deja.com");
  aboutData.addAuthor("Anders Lund", I18N_NOOP("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  aboutData.addAuthor("Matt Newell", I18N_NOOP("Testing, ..."), "newellm@proaxis.com");
  aboutData.addAuthor("Joseph Wenninger", I18N_NOOP("Core Developer"), "jowenn@bigfoot.com");
  aboutData.addAuthor("Michael McCallum", I18N_NOOP("Core Developer"), "gholam@xtra.co.nz");
  aboutData.addAuthor( "Jochen Wilhemly", I18N_NOOP( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  aboutData.addAuthor( "Michael Koch",I18N_NOOP("KWrite port to KParts"), "koch@kde.org");
  aboutData.addAuthor( "Christian Gebauer", 0, "gebauer@bigfoot.com" );
  aboutData.addAuthor( "Simon Hausmann", 0, "hausmann@kde.org" );
  aboutData.addAuthor("Glen Parker",I18N_NOOP("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  aboutData.addAuthor("Scott Manson",I18N_NOOP("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");

  KCmdLineArgs::init (argc, argv, &aboutData);
  KCmdLineArgs::addCmdLineOptions (options);
  KantApp::addCmdLineOptions ();

  if (!KantApp::start()) {
    kdDebug(13000)<<"main: cant start kantapp!"<<endl;
    exit(0);
  }

  KantApp app;
  return app.exec();
}
