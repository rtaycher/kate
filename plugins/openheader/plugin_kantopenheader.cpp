/***************************************************************************
                          plugin_kanttextfilter.cpp  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "plugin_kantopenheader.h"
#include "plugin_kantopenheader.moc"

#include <qinputdialog.h>
#include <kaction.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <cassert>
#include <kdebug.h>
#include <qstring.h>
#include <kurl.h>
#include <kio/netaccess.h>

extern "C"
{
  void* init_libkantopenheaderplugin()
  {
    return new KantPluginFactory;
  }
}

KantPluginFactory::KantPluginFactory()
{
  s_instance = new KInstance( "kant" );
}

KantPluginFactory::~KantPluginFactory()
{
  delete s_instance;
}

QObject* KantPluginFactory::createObject( QObject* parent, const char* name, const char*, const QStringList & )
{
  return new PluginKantOpenHeader( parent, name );
}

KInstance* KantPluginFactory::s_instance = 0L;

PluginKantOpenHeader::PluginKantOpenHeader( QObject* parent, const char* name )
    : KantPluginIface ( parent, name )
{
}

PluginKantOpenHeader::~PluginKantOpenHeader()
{
}

KantPluginViewIface *PluginKantOpenHeader::createView ()
{
   KantPluginViewIface *view = new KantPluginViewIface ();

   (void)  new KAction ( i18n("Open .h/[.cpp.c]"), "file_openheader", 0, this,
  SLOT( slotOpenHeader() ), view->actionCollection(), "file_openheader" );

   view->setXML( "plugins/kantopenheader/ui.rc" );
   viewList.append (view);
   return view;
}


void PluginKantOpenHeader::slotOpenHeader ()
{
  KantViewIface * kv (appIface->viewManagerIface()->getActiveView());
  if (!kv) return;
  KURL url=kv->document()->url();
  if ((url.isMalformed()) || (url.isEmpty())) return;
  QString docname=url.url();
  bool showh=true;
  int len=0;
  if (docname.right(2).upper()==".C") len=2;
  else if (docname.right(4).upper()==".CPP") len=4;
  else if (docname.right(2).upper()==".H") {len=2; showh=false;}
  else return;
  docname=docname.left(docname.length()-len);
  if (showh)
    {
      if (KIO::NetAccess::exists(docname+".h"))  appIface->viewManagerIface()->openURL(KURL(docname+".h"));
      else appIface->viewManagerIface()->openURL(KURL(docname+'H'));
    }
  else
    {
      if (KIO::NetAccess::exists(docname+".c"))  appIface->viewManagerIface()->openURL(KURL(docname+".c"));
      else if (KIO::NetAccess::exists(docname+".cpp"))  appIface->viewManagerIface()->openURL(KURL(docname+".cpp"));
        else if (KIO::NetAccess::exists(docname+".C"))  appIface->viewManagerIface()->openURL(KURL(docname+".C"));
          else appIface->viewManagerIface()->openURL(KURL(docname+".CPP"));
    }
}
