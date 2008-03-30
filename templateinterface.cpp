/* This file is part of the KDE libraries
  Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License version 2 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "templateinterface.h"
#include "document.h"
#include "view.h"
#include <QtCore/QString>
#include <klocale.h>
#include <kglobal.h>
#include <QtCore/QDate>
#include <QtCore/QRegExp>
#include <kmessagebox.h>
#include <kcalendarsystem.h>
#include <unistd.h>
#include <klibloader.h>

#include <kdebug.h>

using namespace KTextEditor;

bool TemplateInterface::expandMacros( QMap<QString, QString> &map, QWidget *parentWindow)
{
  QDateTime datetime = QDateTime::currentDateTime();
  QDate date = datetime.date();
  QTime time = datetime.time();
  typedef QString (*kabcbridgecalltype)(const QString&,QWidget *,bool *ok);
  kabcbridgecalltype kabcbridgecall=0;

  QStringList kabcitems;
  kabcitems<<"firstname"<<"lastname"<<"fullname"<<"email";

  QMap<QString,QString>::Iterator it;
  for ( it = map.begin(); it != map.end(); ++it )
  {
    QString placeholder = it.key();
    if ( map[ placeholder ].isEmpty() )
    {
      if ( placeholder == "index" ) map[ placeholder ] = "i";
      else if ( placeholder == "loginname" )
      {}
      else if (kabcitems.contains(placeholder))
      {
        if (kabcbridgecall==0)
        {
          KLibrary *lib=KLibLoader::self()->library(QLatin1String("ktexteditorkabcbridge"));
          if (lib)
              kabcbridgecall=(kabcbridgecalltype)lib->resolveFunction("ktexteditorkabcbridge");
          if (kabcbridgecall == 0)
          {
            KMessageBox::sorry(parentWindow,i18n("The template needs information about you, which is stored in your address book.\nHowever, the required plugin could not be loaded.\n\nPlease install the KDEPIM/Kontact package for your system."));
            return false;
          }
        }
        bool ok;
        map[ placeholder ] = kabcbridgecall(placeholder,parentWindow,&ok);
        if (!ok)
        {
          return false;
        }
      }
      else if ( placeholder == "date" )
      {
        map[ placeholder ] = KGlobal::locale() ->formatDate( date, KLocale::ShortDate );
      }
      else if ( placeholder == "time" )
      {
        map[ placeholder ] = KGlobal::locale() ->formatTime( time, true, false );
      }
      else if ( placeholder == "year" )
      {
        map[ placeholder ] = KGlobal::locale() ->calendar() ->yearString( date, KCalendarSystem::LongFormat );
      }
      else if ( placeholder == "month" )
      {
        map[ placeholder ] = QString::number( KGlobal::locale() ->calendar() ->month( date ) );
      }
      else if ( placeholder == "day" )
      {
        map[ placeholder ] = QString::number( KGlobal::locale() ->calendar() ->day( date ) );
      }
      else if ( placeholder == "hostname" )
      {
        char hostname[ 256 ];
        hostname[ 0 ] = 0;
        gethostname( hostname, 255 );
        hostname[ 255 ] = 0;
        map[ placeholder ] = QString::fromLocal8Bit( hostname );
      }
      else if ( placeholder == "cursor" )
      {
        map[ placeholder ] = "|";
      }
      else map[ placeholder ] = placeholder;
    }
  }
  return true;
}

bool TemplateInterface::insertTemplateText ( const Cursor& insertPosition, const QString &templateString, const QMap<QString, QString> &initialValues)
{
  QMap<QString, QString> enhancedInitValues( initialValues );

  QRegExp rx( "[$%]\\{([^}\\s]+)\\}" );
  rx.setMinimal( true );
  int pos = 0;
  int opos = 0;

  while ( pos >= 0 )
  {
    pos = rx.indexIn( templateString, pos );

    if ( pos > -1 )
    {
      if ( ( pos - opos ) > 0 )
      {
        if ( templateString[ pos - 1 ] == '\\' )
        {
          pos = opos = pos + 1;
          continue;
        }
      }
      QString placeholder = rx.cap( 1 );
      if ( ! enhancedInitValues.contains( placeholder ) )
        enhancedInitValues[ placeholder ] = "";

      pos += rx.matchedLength();
      opos = pos;
    }
  }

  return expandMacros( enhancedInitValues, dynamic_cast<QWidget*>(this) )
         && insertTemplateTextImplementation( insertPosition, templateString, enhancedInitValues);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
