/***************************************************************************
                          katemain.cpp  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kstandarddirs.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kglobal.h>
#include <kstartupinfo.h>
#include <dcopclient.h>
#include <kurl.h>

#include "kateapp.h"

#include <stdio.h>

static KCmdLineOptions options[] =
{
    { "n", I18N_NOOP("start a new Kate (off by default)"), 0 },
    { "s", I18N_NOOP("start Kate in SDI mode (off by default)"), 0 },
    { "+file(s)",          I18N_NOOP("Files to load"), 0 },
    { 0,0,0 }
};

int main( int argc, char **argv )
{
  KAboutData *data = new KAboutData  ("kate", I18N_NOOP("Kate"), "2.0",
                                                           I18N_NOOP( "Kate - KDE Advanced Text Editor" ), KAboutData::License_GPL,
                                                           I18N_NOOP( "(c) 2000-2001 The Kate Authors" ), 0, "http://kate.sourceforge.net");

  data->addAuthor ("Christoph Cullmann", I18N_NOOP("Project Manager and Core Developer"), "cullmann@kde.org", "http://www.babylon2k.de");
  data->addAuthor ("Anders Lund", I18N_NOOP("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  data->addAuthor ("Joseph Wenninger", I18N_NOOP("Core Developer"), "jowenn@kde.org","http://stud3.tuwien.ac.at/~e9925371");
  data->addAuthor ("Michael Bartl", I18N_NOOP("Core Developer"), "michael.bartl1@chello.at");
  data->addAuthor ("Phlip", I18N_NOOP("The Project Compiler"), "phlip_cpp@my-deja.com");
  data->addAuthor ("Waldo Bastian", I18N_NOOP( "The cool buffersystem" ), "bastian@kde.org" );
  data->addAuthor ("Matt Newell", I18N_NOOP("Testing, ..."), "newellm@proaxis.com");
  data->addAuthor ("Michael McCallum", I18N_NOOP("Core Developer"), "gholam@xtra.co.nz");
  data->addAuthor ("Jochen Wilhemly", I18N_NOOP( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  data->addAuthor ("Michael Koch",I18N_NOOP("KWrite port to KParts"), "koch@kde.org");
  data->addAuthor ("Christian Gebauer", 0, "gebauer@kde.org" );
  data->addAuthor ("Simon Hausmann", 0, "hausmann@kde.org" );
  data->addAuthor ("Glen Parker",I18N_NOOP("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  data->addAuthor ("Scott Manson",I18N_NOOP("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
  data->addAuthor ("John Firebaugh",I18N_NOOP("Patches and more"), "jfirebaugh@kde.org");

  data->addCredit ("Matteo Merli",I18N_NOOP("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
  data->addCredit ("Rocky Scaletta",I18N_NOOP("Highlighting for VHDL"), "rocky@purdue.edu");
  data->addCredit ("Yury Lebedev",I18N_NOOP("Highlighting for SQL"),"");
  data->addCredit ("Chris Ross",I18N_NOOP("Highlighting for Ferite"),"");
  data->addCredit ("Nick Roux",I18N_NOOP("Highlighting for ILERPG"),"");
  data->addCredit ("John Firebaugh",I18N_NOOP("Highlighting for Java, and much more"),"");
  data->addCredit ("Carsten Niehaus", I18N_NOOP("Highlighting for LaTeX"),"");
  data->addCredit ("Per Wigren", I18N_NOOP("Highlighting for Makefiles, Python"),"");
  data->addCredit ("Jan Fritz", I18N_NOOP("Highlighting for Python"),"");
  data->addCredit ("Daniel Naber","","");
  data->addCredit ("Roland Pabel",I18N_NOOP("Highlighting for Scheme"),"");
  data->addCredit ("Cristi Dumitrescu",I18N_NOOP("PHP Keyword/Datatype list"),"");
  data->addCredit ("Carsten Presser", I18N_NOOP("Betatest"), "mord-slime@gmx.de");
  data->addCredit ("Jens Haupert", I18N_NOOP("Betatest"), "al_all@gmx.de");
  data->addCredit ("Carsten Pfeiffer", I18N_NOOP("Very nice help"), "");
  data->addCredit ("Mr. Doerr", I18N_NOOP("For absolutely nothing"), "");
  data->addCredit (I18N_NOOP("All people who have contributed and I have forgotten to mention"),"","");

  data->setTranslator(I18N_NOOP("_: NAME OF TRANSLATORS\nYour names"), I18N_NOOP("_: EMAIL OF TRANSLATORS\nYour emails"));

  KCmdLineArgs::init (argc, argv, data);
  KCmdLineArgs::addCmdLineOptions (options);
  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  bool running = false;

  DCOPClient *client=0L, *client2=0L;
  QCString appID = "";

  if (!args->isSet ("n")) // && !args->isSet ("s"))
  {
    client  = new DCOPClient ();
    client->attach();

    QCStringList apps = client->registeredApplications();
    for ( QCStringList::Iterator it = apps.begin(); it != apps.end(); ++it )
    {
      if  ((*it).contains ("kate") > 0)
      {
        appID = (*it);

        QByteArray ba,da;
        QCString replyType;
        if (!(client->call(appID,"KateApp","isSingleInstance()",da,replyType,ba,true)))
	  running = false;
        else
          {
            if (replyType!="QString") running=false;
              else
                {
		   QDataStream reply(ba, IO_ReadOnly);
                   QString result;
                   reply>>result;
		   running=(result=="true");
                }
          }
	break;
      }
    }
  }

  if (running)
  {
    for (int z=0; z<args->count(); z++)
    {
      QByteArray data;
      QDataStream arg(data, IO_WriteOnly);

      arg << args->url(z).url();
      client->send (appID, "KateApp", "openURL(QString)", data);
    }

    QByteArray data;
    client->send (appID, "KateApp", "raiseCurrentMainWindow()", data);

    // only to let klauncher not fail on a second instance of kate ;)
    client2  = new DCOPClient ();
    client2->registerAs("kate");

    KStartupInfo::appStarted();
  }
  else
  {
    KateApp::addCmdLineOptions ();
    KateApp app;
    app._isSDI = args->isSet ("s");
    return app.exec();
  }

  return 0;
}
