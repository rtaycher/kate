
#include "kwrite_factory.h"

#include "kwdoc.h"
#include "highlight.h"
#include "kwview.h"

#include <klocale.h>
#include <kinstance.h>
#include <kaboutdata.h>

extern "C"
{
  void *init_libkwritepart()
  {
    return new KWriteFactory();
  }
}

KInstance *KWriteFactory::s_instance = 0;
KAboutData *KWriteFactory::s_aboutData = 0;

KWriteFactory::KWriteFactory()
{
  s_instance = 0; // I don't trust anyone ;-)
  s_aboutData = 0;
}

KWriteFactory::~KWriteFactory()
{
  if ( s_instance )
    delete s_instance;
  if ( s_aboutData )
    delete s_aboutData;
  s_instance = 0;
  s_aboutData = 0;
}

KParts::Part *KWriteFactory::createPart( QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name, const char *classname, const QStringList & )
{
  bool bWantDocument = ( strcmp( classname, "KTextEditor::Document" ) == 0 );
  bool bWantBrowserView = ( strcmp( classname, "Browser/View" ) == 0 );

  KParts::ReadWritePart *part = new KWriteDoc( HlManager::self(), QString::null, !bWantDocument, bWantBrowserView, parentWidget, widgetName, parent, name );

  if ( bWantBrowserView || ( strcmp( classname, "KParts::ReadOnlyPart" ) == 0 ) )
    part->setReadWrite( false );

  emit objectCreated( part );
  return part;
}

KInstance *KWriteFactory::instance()
{
  if ( !s_instance )
    s_instance = new KInstance( aboutData() );
  return s_instance;
}

const KAboutData *KWriteFactory::aboutData()
{
  if ( !s_aboutData )
  {
    s_aboutData = new KAboutData( "kwrite", I18N_NOOP( "KWrite" ),
				  KWRITE_VERSION,
				  I18N_NOOP( "Advanced Texteditor Component" ),
				  KAboutData::License_GPL,

				  "(c) 2000, Jochen Wilhelmy" );
    s_aboutData->addAuthor( "Jochen Wilhemly", I18N_NOOP( "Author" ), "digisnap@cs.tu-berlin.de" );
    s_aboutData->addAuthor("Michael Koch",I18N_NOOP("Port to KParts"), "koch@kde.org");
    s_aboutData->addAuthor("Glen Parker",I18N_NOOP("Undo History, Kspell integration"), "glenebob@nwlink.com");

  }

  return s_aboutData;
}

#include "kwrite_factory.moc"
