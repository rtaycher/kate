/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __kate_pluginconfiginterfaceextension_h__
#define __kate_pluginconfiginterfaceextension_h__

#include <qwidget.h>
#include <qpixmap.h>
#include <kicontheme.h>

namespace Kate
{

class PluginConfigPage : public QWidget
{
  Q_OBJECT

  public:
    PluginConfigPage ( QWidget *parent=0, const char *name=0 );
    virtual ~PluginConfigPage ();

  //
  // slots !!!
  //
  public:
    /**
      Applies the changes to the document
    */
    virtual void apply () = 0;
    
    /**
      Reset the changes
    */
    virtual void reset () = 0;
    
    /**
      Sets default options
    */
    virtual void defaults () = 0;
};

/*
*  This is an interface for the KTextEditor::Document/Plugin/ViewPlugin classes !!!
*/
class PluginConfigInterfaceExtension
{
  friend class PrivatePluginConfigInterfaceExtension;

  public:
    PluginConfigInterfaceExtension();
    virtual ~PluginConfigInterfaceExtension();

    unsigned int pluginConfigInterfaceExtensionNumber () const;

  //
  // slots !!!
  //
  public:    
    virtual PluginConfigPage *configPage (QWidget *parent = 0, const char *name=0 ) = 0;
  
    virtual QString configPageName () const = 0;
    virtual QString configPageFullName () const = 0;
    virtual QPixmap configPagePixmap (int size = KIcon::SizeSmall) const = 0;
    
    
  private:
    class PrivatePluginConfigInterfaceExtension *d;
    static unsigned int globalPluginConfigInterfaceExtensionNumber;
    unsigned int myPluginConfigInterfaceExtensionNumber;
};

class Plugin;
PluginConfigInterfaceExtension *pluginConfigInterfaceExtension (Plugin *plugin);

};

#endif
