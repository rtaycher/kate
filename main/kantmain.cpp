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

#include "../app/kantapp.h"
#include "../factory/kantfactory.h"

static KCmdLineOptions options[] =
{
    { "+file(s)",          I18N_NOOP("Files to load"), 0 },
    { 0,0,0 }
};

int main( int argc, char **argv )
{
  KCmdLineArgs::init (argc, argv, KantFactory::aboutData());
  KCmdLineArgs::addCmdLineOptions (options);
  KantApp::addCmdLineOptions ();

  if (!KantApp::start())
  {
    kdDebug(13000)<<"main: cant start kantapp!"<<endl;
    return 1;
  }

  KantApp app;
  return app.exec();
}
