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

// $Id$

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
    { "initplugin <argument>",	I18N_NOOP("Allow Kate to be initialized by a plugin. You most probably have to specify a file too."),0},
    { "line <argument>",      I18N_NOOP("Navigate to this line"), 0 },
    { "column <argument>",      I18N_NOOP("Navigate to this column"), 0 },
    { "+file(s)",          I18N_NOOP("Files to load"), 0 },
    KCmdLineLastOption
};

extern "C" int kdemain( int argc, char **argv )
{
  Kate::Document::setFileChangedDialogsActivated (true);

  KAboutData *s_about = new KAboutData  ("kate", I18N_NOOP("Kate"), "2.2",
                                                           I18N_NOOP( "Kate - Advanced Text Editor" ), KAboutData::License_LGPL_V2,
                                                           I18N_NOOP( "(c) 2000-2003 The Kate Authors" ), 0, "http://kate.kde.org");

    s_about->addAuthor ("Christoph Cullmann", I18N_NOOP("Maintainer"), "cullmann@kde.org", "http://www.babylon2k.de");
    s_about->addAuthor ("Anders Lund", I18N_NOOP("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
    s_about->addAuthor ("Joseph Wenninger", I18N_NOOP("Core Developer"), "jowenn@kde.org","http://stud3.tuwien.ac.at/~e9925371");
    s_about->addAuthor ("Hamish Rodda",I18N_NOOP("Core Developer"), "meddie@yoyo.its.monash.edu.au");
    s_about->addAuthor ("Waldo Bastian", I18N_NOOP( "The cool buffersystem" ), "bastian@kde.org" );
    s_about->addAuthor ("Charles Samuels", I18N_NOOP("The Editing Commands"), "charles@kde.org");
    s_about->addAuthor ("Matt Newell", I18N_NOOP("Testing, ..."), "newellm@proaxis.com");
    s_about->addAuthor ("Michael Bartl", I18N_NOOP("Former Core Developer"), "michael.bartl1@chello.at");
    s_about->addAuthor ("Michael McCallum", I18N_NOOP("Core Developer"), "gholam@xtra.co.nz");
    s_about->addAuthor ("Jochen Wilhemly", I18N_NOOP( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
    s_about->addAuthor ("Michael Koch",I18N_NOOP("KWrite port to KParts"), "koch@kde.org");
    s_about->addAuthor ("Christian Gebauer", 0, "gebauer@kde.org" );
    s_about->addAuthor ("Simon Hausmann", 0, "hausmann@kde.org" );
    s_about->addAuthor ("Glen Parker",I18N_NOOP("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
    s_about->addAuthor ("Scott Manson",I18N_NOOP("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
    s_about->addAuthor ("John Firebaugh",I18N_NOOP("Patches and more"), "jfirebaugh@kde.org");

    s_about->addCredit ("Matteo Merli",I18N_NOOP("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
    s_about->addCredit ("Rocky Scaletta",I18N_NOOP("Highlighting for VHDL"), "rocky@purdue.edu");
    s_about->addCredit ("Yury Lebedev",I18N_NOOP("Highlighting for SQL"),"");
    s_about->addCredit ("Chris Ross",I18N_NOOP("Highlighting for Ferite"),"");
    s_about->addCredit ("Nick Roux",I18N_NOOP("Highlighting for ILERPG"),"");
    s_about->addCredit ("Carsten Niehaus", I18N_NOOP("Highlighting for LaTeX"),"");
    s_about->addCredit ("Per Wigren", I18N_NOOP("Highlighting for Makefiles, Python"),"");
    s_about->addCredit ("Jan Fritz", I18N_NOOP("Highlighting for Python"),"");
    s_about->addCredit ("Daniel Naber","","");
    s_about->addCredit ("Roland Pabel",I18N_NOOP("Highlighting for Scheme"),"");
    s_about->addCredit ("Cristi Dumitrescu",I18N_NOOP("PHP Keyword/Datatype list"),"");
    s_about->addCredit ("Carsten Presser", I18N_NOOP("Betatest"), "mord-slime@gmx.de");
    s_about->addCredit ("Jens Haupert", I18N_NOOP("Betatest"), "al_all@gmx.de");
    s_about->addCredit ("Carsten Pfeiffer", I18N_NOOP("Very nice help"), "");
    s_about->addCredit (I18N_NOOP("All people who have contributed and I have forgotten to mention"),"","");

    s_about->setTranslator(I18N_NOOP("_: NAME OF TRANSLATORS\nYour names"), I18N_NOOP("_: EMAIL OF TRANSLATORS\nYour emails"));

  KCmdLineArgs::init (argc, argv, s_about);
  KCmdLineArgs::addCmdLineOptions (options);
  KateApp::addCmdLineOptions ();
  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  bool newProcess = false;
  bool oldState = false;

  if (args->isSet ("n"))
    newProcess = true;

  if (newProcess)
  {
    KInstance *instance = new KInstance (s_about);

    KConfig *config = instance->config();
    config->setGroup("KDE");
    oldState = config->readBoolEntry("MultipleInstances",false);
    config->writeEntry("MultipleInstances",true);
    config->sync();

    delete instance;
  }

  KateApp app (newProcess, oldState);
  return app.exec();
}
