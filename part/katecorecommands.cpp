/* This file is part of the KDE libraries
   Copyright (C) 2003 Anders Lund <anders@alweb.dk>

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

#include "katecorecommands.h"

#include "kateview.h"
#include "katedocument.h"

#include <klocale.h>

#include <qregexp.h>

// syncs a config flag in the document with a boolean value
static void setDocFlag( KateDocumentConfig::ConfigFlags flag, bool enable,
                  KateDocument *doc )
{
  doc->config()->setConfigFlags( flag, enable );
}

QStringList KateCoreCommands::cmds()
{
  QStringList l;
  l << "indent" << "unindent" << "cleanindent"
    << "comment" << "uncomment"
    << "set-tab-width" << "set-replace-tabs" << "set-show-tabs"
    << "set-indent-spaces" << "set-indent-width" << "set-indent-mode" << "set-auto-indent"
    << "set-line-numbers" << "set-folding-markers" << "set-icon-border"
    << "set-word-wrap" << "set-word-wrap-column";
  return l;
}

bool KateCoreCommands::exec(Kate::View *view,
                            const QString &_cmd,
                            QString &errorMsg)
{
#define KCC_ERR(s) { errorMsg=s; return false; }
  KateView *v = dynamic_cast<KateView*>( view );
  if ( ! v )
    KCC_ERR( i18n("Could not access view") );

  //create a list of args
  QStringList args( QStringList::split( QRegExp("\\s+"), _cmd ) );
  QString cmd ( args.first() );
  args.remove( args.first() );

  // ALL commands that takes no arguments.
  if ( cmd == "indent" )
  {
    v->indent();
    return true;
  }
  else if ( cmd == "unindent" )
  {
    v->unIndent();
    return true;
  }
  else if ( cmd == "cleanindent" )
  {
    v->cleanIndent();
    return true;
  }
  else if ( cmd == "comment" )
  {
    v->comment();
    return true;
  }
  else if ( cmd == "uncomment" )
  {
    v->uncomment();
    return true;
  }
  else if ( cmd == "set-indent-mode" )
  {
    bool ok(false);
    int val ( args.first().toInt( &ok ) );
    if ( ok )
    {
      if ( val < 0 )
        KCC_ERR( i18n("Mode must be at least 0.") );
      v->doc()->config()->setIndentationMode( val );
    }
    else
      v->doc()->config()->setIndentationMode( KateAutoIndent::modeNumber( args.first() ) );
    return true;
  }

  // ALL commands that takes exactly one integer argument.
  else if ( cmd == "set-tab-width" ||
            cmd == "set-indent-width" ||
            cmd == "set-word-wrap-column" )
  {
    // find a integer value > 0
    if ( ! args.count() )
      KCC_ERR( i18n("Missing argument. Usage: %1 <value>").arg( cmd ) );
    bool ok;
    int val ( args.first().toInt( &ok ) );
    if ( !ok )
      KCC_ERR( i18n("Failed to convert argument '%1' to integer.")
                .arg( args.first() ) );

    if ( cmd == "set-tab-width" )
    {
      if ( val < 1 )
        KCC_ERR( i18n("Width must be at least 1.") );
      v->setTabWidth( val );
    }
    else if ( cmd == "set-indent-width" )
    {
      if ( val < 1 )
        KCC_ERR( i18n("Width must be at least 1.") );
      v->doc()->config()->setIndentationWidth( val );
    }
    else if ( cmd == "set-word-wrap-column" )
    {
      if ( val < 2 )
        KCC_ERR( i18n("Column must be at least 1.") );
      v->doc()->setWordWrapAt( val );
    }
    return true;
  }

  // ALL commands that takes 1 boolean argument.
  else if ( cmd == "set-icon-border" ||
            cmd == "set-folding-markers" ||
            cmd == "set-line-numbers" ||
            cmd == "set-replace-tabs" ||
            cmd == "set-show-tabs" ||
            cmd == "set-indent-spaces" ||
            cmd == "set-auto-indent" ||
            cmd == "set-word-wrap" )
  {
    if ( ! args.count() )
      KCC_ERR( i18n("Usage: %1 on|off|1|0|true|false").arg( cmd ) );
    bool enable;
    if ( getBoolArg( args.first(), &enable ) )
    {
      if ( cmd == "set-icon-border" )
        v->setIconBorder( enable );
      else if (cmd == "set-folding-markers")
        v->setFoldingMarkersOn( enable );
      else if ( cmd == "set-line-numbers" )
        v->setLineNumbersOn( enable );
      else if ( cmd == "set-replace-tabs" )
        setDocFlag( KateDocumentConfig::cfReplaceTabs, enable, v->doc() );
      else if ( cmd == "set-show-tabs" )
        setDocFlag( KateDocumentConfig::cfShowTabs, enable, v->doc() );
      else if ( cmd == "set-indent-spaces" )
        setDocFlag( KateDocumentConfig::cfSpaceIndent, enable, v->doc() );
      else if ( cmd == "set-auto-indent" )
        setDocFlag( KateDocumentConfig::cfAutoIndent, enable, v->doc() );
      else if ( cmd == "set-word-wrap" )
        v->doc()->setWordWrap( enable );

      return true;
    }
    else
      KCC_ERR( i18n("Bad argument '%1'. Usage: %2 on|off|1|0|true|false")
               .arg( args.first() ).arg( cmd ) );
  }

  // unlikely..
  KCC_ERR( i18n("Unknown command '%1'").arg(cmd) );
}

// this returns wheather the string s could be converted to
// a bool value, one of on|off|1|0|true|false. the argument val is
// set to the extracted value in case of success
bool KateCoreCommands::getBoolArg( QString s, bool *val  )
{
  bool res( false );
  s = s.lower();
  res = (s == "on" || s == "1" || s == "true");
  if ( res )
  {
    *val = true;
    return true;
  }
  res = (s == "off" || s == "0" || s == "false");
  if ( res )
  {
    *val = false;
    return true;
  }
  return false;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
