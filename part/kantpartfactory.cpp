/***************************************************************************
                          kantpartfactory.cpp  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
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

#include "kantpartfactory.h"
#include "kantpartfactory.moc"

#include "kantpartdocument.h"
#include "../document/kantdocument.h"

#include <klocale.h>
#include <kinstance.h>
#include <kaboutdata.h>

extern "C"
{
  void *init_libkantpart()
  {
    return new KantPartFactory();
  }
}

KInstance *KantPartFactory::s_instance = 0L;

KantPartFactory::KantPartFactory()
{
  s_instance = 0L;
}

KantPartFactory::~KantPartFactory()
{
  if ( s_instance )
  {
    delete s_instance->aboutData();
    delete s_instance;
  }
  s_instance = 0L;
}

KParts::Part *KantPartFactory::createPart( QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name, const char *classname, const QStringList & )
{
  bool bWantDocument = ( strcmp( classname, "KTextEditor::Document" ) == 0L );
  bool bWantBrowserView = ( strcmp( classname, "Browser/View" ) == 0L );

  KParts::ReadWritePart *part = new KantPartDocument ( !bWantDocument, bWantBrowserView, parentWidget, widgetName, parent, name );

  if ( bWantBrowserView || ( strcmp( classname, "KParts::ReadOnlyPart" ) == 0 ) )
    part->setReadWrite( false );

  emit objectCreated( part );
  return part;
}

KInstance *KantPartFactory::instance()
{
  if ( !s_instance )
    s_instance = new KInstance( aboutData() );
  return s_instance;
}

const KAboutData *KantPartFactory::aboutData()
{
  KAboutData *data = new KAboutData  ("kant", I18N_NOOP("Kant Part"), "0.2",
                                                           I18N_NOOP( "Kant Part - A Texteditor KPart for KDE" ), KAboutData::License_GPL,
                                                           "(c) 2000-2001 The Kant Authors", "http://devel-home.kde.org/~kant");

  data->addAuthor("Christoph Cullmann", I18N_NOOP("Project Manager and Core Developer"), "crossfire@babylon2k.de", "http://www.babylon2k.de");
  data->addAuthor("Michael Bartl", I18N_NOOP("Core Developer"), "michael.bartl1@chello.at");
  data->addAuthor("Phlip", I18N_NOOP("The Project Compiler"), "phlip_cpp@my-deja.com");
  data->addAuthor("Anders Lund", I18N_NOOP("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  data->addAuthor("Matt Newell", I18N_NOOP("Testing, ..."), "newellm@proaxis.com");
  data->addAuthor("Joseph Wenninger", I18N_NOOP("Core Developer"), "jowenn@bigfoot.com");
  data->addAuthor("Michael McCallum", I18N_NOOP("Core Developer"), "gholam@xtra.co.nz");
  data->addAuthor( "Jochen Wilhemly", I18N_NOOP( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  data->addAuthor( "Michael Koch",I18N_NOOP("KWrite port to KParts"), "koch@kde.org");
  data->addAuthor( "Christian Gebauer", 0, "gebauer@bigfoot.com" );
  data->addAuthor( "Simon Hausmann", 0, "hausmann@kde.org" );
  data->addAuthor("Glen Parker",I18N_NOOP("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  data->addAuthor("Scott Manson",I18N_NOOP("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");

  return data;
}
