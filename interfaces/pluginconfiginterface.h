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

#ifndef __kate_pluginconfiginterface_h__
#define __kate_pluginconfiginterface_h__

namespace Kate
{

/*
*  This is an interface for the KTextEditor::Document/Plugin/ViewPlugin classes !!!
*/
class PluginConfigInterface
{
  friend class PrivatePluginConfigInterface;

  public:
    PluginConfigInterface();
    virtual ~PluginConfigInterface();

    unsigned int pluginConfigInterfaceNumber () const;

  //
  // slots !!!
  //
  public:    
    /**
      Read/Write the config to the standard place where this editor
      part saves it config, say: read/save default values for that
      editor part
    */
    virtual void readConfig () = 0;
    virtual void writeConfig () = 0;           
                                                                             
  private:
    class PrivatePluginConfigInterface *d;
    static unsigned int globalPluginConfigInterfaceNumber;
    unsigned int myPluginConfigInterfaceNumber;
};

class Plugin;
PluginConfigInterface *pluginConfigInterface (Plugin *plugin);

};

#endif
