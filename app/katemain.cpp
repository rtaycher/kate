/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

#include <kstandarddirs.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kinstance.h>

#include "kateapp.h"

static KCmdLineOptions options[] =
{
    { "n", I18N_NOOP("Start a new Kate process (off by default)"), 0 },
    { "w", I18N_NOOP("Open a new Kate window"), 0 },
    { "s", 0 , 0 },
    { "start-session <argument>",      I18N_NOOP("Start Kate with a given session, starts new kate process, too. If session exists, it will be loaded, else it will be created."), 0 },
    { "initplugin <argument>",	I18N_NOOP("Allow Kate to be initialized by a plugin. You most probably have to specify a file too."),0},
    { "encoding <argument>",      I18N_NOOP("Set encoding for the file to open"), 0 },
    { "line <argument>",      I18N_NOOP("Navigate to this line"), 0 },
    { "column <argument>",      I18N_NOOP("Navigate to this column"), 0 },
    { "+[URL]",          I18N_NOOP("Document to open"), 0 },
    KCmdLineLastOption
};

extern "C" KDE_EXPORT int kdemain( int argc, char **argv )
{
  Kate::Document::setFileChangedDialogsActivated (true);

  KAboutData aboutData ("kate", I18N_NOOP("Kate"), KATE_VERSION,
                        I18N_NOOP( "Kate - Advanced Text Editor" ), KAboutData::License_LGPL_V2,
                        I18N_NOOP( "(c) 2000-2004 The Kate Authors" ), 0, "http://kate.kde.org");

  aboutData.addAuthor ("Christoph Cullmann", I18N_NOOP("Maintainer"), "cullmann@kde.org", "http://www.babylon2k.de");
  aboutData.addAuthor ("Anders Lund", I18N_NOOP("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  aboutData.addAuthor ("Joseph Wenninger", I18N_NOOP("Core Developer"), "jowenn@kde.org","http://stud3.tuwien.ac.at/~e9925371");
  aboutData.addAuthor ("Hamish Rodda",I18N_NOOP("Core Developer"), "rodda@kde.org");
  aboutData.addAuthor ("Waldo Bastian", I18N_NOOP( "The cool buffersystem" ), "bastian@kde.org" );
  aboutData.addAuthor ("Charles Samuels", I18N_NOOP("The Editing Commands"), "charles@kde.org");
  aboutData.addAuthor ("Matt Newell", I18N_NOOP("Testing, ..."), "newellm@proaxis.com");
  aboutData.addAuthor ("Michael Bartl", I18N_NOOP("Former Core Developer"), "michael.bartl1@chello.at");
  aboutData.addAuthor ("Michael McCallum", I18N_NOOP("Core Developer"), "gholam@xtra.co.nz");
  aboutData.addAuthor ("Jochen Wilhemly", I18N_NOOP( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  aboutData.addAuthor ("Michael Koch",I18N_NOOP("KWrite port to KParts"), "koch@kde.org");
  aboutData.addAuthor ("Christian Gebauer", 0, "gebauer@kde.org" );
  aboutData.addAuthor ("Simon Hausmann", 0, "hausmann@kde.org" );
  aboutData.addAuthor ("Glen Parker",I18N_NOOP("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  aboutData.addAuthor ("Scott Manson",I18N_NOOP("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
  aboutData.addAuthor ("John Firebaugh",I18N_NOOP("Patches and more"), "jfirebaugh@kde.org");
  aboutData.addAuthor ("Dominik Haumann", I18N_NOOP("Developer & Highlight wizard"), "dhdev@gmx.de");

  aboutData.addCredit ("Matteo Merli",I18N_NOOP("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
  aboutData.addCredit ("Rocky Scaletta",I18N_NOOP("Highlighting for VHDL"), "rocky@purdue.edu");
  aboutData.addCredit ("Yury Lebedev",I18N_NOOP("Highlighting for SQL"),"");
  aboutData.addCredit ("Chris Ross",I18N_NOOP("Highlighting for Ferite"),"");
  aboutData.addCredit ("Nick Roux",I18N_NOOP("Highlighting for ILERPG"),"");
  aboutData.addCredit ("Carsten Niehaus", I18N_NOOP("Highlighting for LaTeX"),"");
  aboutData.addCredit ("Per Wigren", I18N_NOOP("Highlighting for Makefiles, Python"),"");
  aboutData.addCredit ("Jan Fritz", I18N_NOOP("Highlighting for Python"),"");
  aboutData.addCredit ("Daniel Naber","","");
  aboutData.addCredit ("Roland Pabel",I18N_NOOP("Highlighting for Scheme"),"");
  aboutData.addCredit ("Cristi Dumitrescu",I18N_NOOP("PHP Keyword/Datatype list"),"");
  aboutData.addCredit ("Carsten Pfeiffer", I18N_NOOP("Very nice help"), "");
  aboutData.addCredit (I18N_NOOP("All people who have contributed and I have forgotten to mention"),"","");

  aboutData.setTranslator(I18N_NOOP("_: NAME OF TRANSLATORS\nYour names"), I18N_NOOP("_: EMAIL OF TRANSLATORS\nYour emails"));

  KCmdLineArgs::init (argc, argv, &aboutData);
  KCmdLineArgs::addCmdLineOptions (options);
  KateApp::addCmdLineOptions ();
  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  bool newProcess = false;
  bool oldState = false;

  // spawn new proccess if session given
  if (args->isSet ("n") || args->isSet ("start-session"))
    newProcess = true;

  if (newProcess)
  {
    KInstance instance (&aboutData);

    KConfig *config = instance.config();
    config->setGroup("KDE");
    oldState = config->readBoolEntry("MultipleInstances",false);
    config->writeEntry("MultipleInstances",true);
    config->sync();
  }

  KateApp app (newProcess, oldState);
  return app.exec();
}
