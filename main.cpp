/*
    $Id$

    Copyright (C) 2000 Michael Koch <koch@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include <kapp.h>
#include <klocale.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>

#include "kwrite_shell.h"

static const char *description = I18N_NOOP( "KDE Advanced editor" );

static const char *version = KWRITE_VERSION;

static KCmdLineOptions options[] =
{
  { "+[file]", I18N_NOOP("File to open"), 0 },
  { 0, 0, 0 }
};

int main( int argc, char** argv )
{
  KAboutData aboutData("kwrite", I18N_NOOP("Advanced Editor"),
    version, description, KAboutData::License_GPL,
    "(c) 1998-2000, KWrite Developers");
  aboutData.addAuthor("Jochen Wilhelmy","Main Developer", "digisnap@cs.tu-berlin.de");
  aboutData.addAuthor("Michael Koch","Port to KParts", "koch@kde.org");
  aboutData.addAuthor("Glen Parker","Undo History, Kspell integration", "glenebob@nwlink.com");
  
  KCmdLineArgs::init( argc, argv, &aboutData );

  KCmdLineArgs::addCmdLineOptions( options );

  KApplication app;

  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  if( args->count() > 0 )
  {
    for( int i = 0 ; i < args->count() ; i++ )
    {
      KWriteShell *shell = new KWriteShell( args->url( i ) );
      shell->show();
    }
  }
  else
  {
    KWriteShell *shell = new KWriteShell;
    shell->show();
  }
  
  app.exec();

  return 0;
}
