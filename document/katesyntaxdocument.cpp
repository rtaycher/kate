/***************************************************************************
    Copyright (C) 2000 Scott Manson
                       SDManson@alltel.net

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


#include "katesyntaxdocument.h"
#include <qfile.h>
#include <kdebug.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qstringlist.h>


SyntaxDocument::SyntaxDocument() : QDomDocument()
{
//  QString syntaxPath = locate("data", "kate/syntax.xml");
  currentFile="";
  setupModeList();

/*
  if( !syntaxPath.isEmpty() )
  {
    QFile f( syntaxPath );
    if ( f.open(IO_ReadOnly) )
    {
      setContent(&f);
      setupModeList();
    }
    else
      KMessageBox::error( 0L, i18n("Can't open %1").arg(syntaxPath) );

    f.close();
  }
  else
    KMessageBox::error( 0L, i18n("File share/apps/kwrite/syntax.xml not found ! Check your installation!") );
*/
}

void SyntaxDocument::setIdentifier(const QString& identifier)
{
 kdDebug()<<"Trying to set identifier to"<<identifier<<endl;
 if (currentFile!=identifier)
   {
       QFile f( identifier );
       if ( f.open(IO_ReadOnly) )
         {
           setContent(&f);
           currentFile=identifier;
         }
       else
         KMessageBox::error( 0L, i18n("Can't open %1").arg(identifier) );
      f.close();
   }
} 

SyntaxDocument::~SyntaxDocument()
{
}

void SyntaxDocument::setupModeList()
{
  kdDebug(13010) << k_funcinfo << endl;
  if (myModeList.count() > 0) return;

      KStandardDirs *dirs = KGlobal::dirs();
      QStringList list=dirs->findAllResources("data","kate/syntax/*.xml",false,true);
      for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
      {
        QFile f(*it);
        if (f.open(IO_ReadOnly))
          {
             kdDebug(13010)<<"Parsing: "<< *it<< endl;
             setContent(&f);
             f.close();
             QDomElement n = documentElement();
             if (!n.isNull())
               {
                 kdDebug(13010)<<"Node not null"<<endl;
                 QDomElement e=n.toElement();
		 kdDebug(13010)<<"Tagname: "<<e.tagName()<<endl;
                 if (e.tagName()=="language")
                   {
                      kdDebug(13010)<<"language found"<<endl;
                      syntaxModeListItem *mli=new syntaxModeListItem;
                      mli->name = e.attribute("name");
                      mli->mimetype = e.attribute("mimetype");
                      mli->extension = e.attribute("extensions");
                      mli->casesensitive = e.attribute("casesensitive");
                      if (mli->casesensitive.isEmpty()) mli->casesensitive="1";
                      mli->identifier = *it;
                      myModeList.append(mli);
                   }
               }
      }
  }
}

SyntaxModeList SyntaxDocument::modeList()
{
  return myModeList;
}


//QStringList& SyntaxDocument::

bool SyntaxDocument::nextGroup(struct syntaxContextData* data)
{
  if(!data) return false;
  if (data->currentGroup.isNull())
    {
      data->currentGroup=data->parent.firstChild().toElement();

    }
  else
    data->currentGroup=data->currentGroup.nextSibling().toElement();
  data->item=QDomElement();
  if (data->currentGroup.isNull()) return false; else return true;
}

bool SyntaxDocument::nextItem(struct syntaxContextData* data)
{
  if(!data) return false;
  if (data->item.isNull())
    {
      data->item=data->currentGroup.firstChild().toElement();
    }
  else
    data->item=data->item.nextSibling().toElement();
  if (data->item.isNull()) return false; else return true;
}

QString SyntaxDocument::groupItemData(struct syntaxContextData* data,QString name)
{
  if(!data) return QString::null;
  if ( (!data->item.isNull()) && (name.isEmpty())) return data->item.tagName();
  if (!data->item.isNull()) return data->item.attribute(name); else return QString();
}

QString SyntaxDocument::groupData(struct syntaxContextData* data,QString name)
{
    if(!data) return QString::null;
    if (!data->currentGroup.isNull()) return data->currentGroup.attribute(name); else return QString();
}

void SyntaxDocument::freeGroupInfo(struct syntaxContextData* data)
{
  if(data)   delete data;
}


struct syntaxContextData* SyntaxDocument::getSubItems(struct syntaxContextData* data)
{
  syntaxContextData *retval=new syntaxContextData;
  if (data!=0)
    {  
      retval->parent=data->currentGroup;
      retval->currentGroup=data->item;
      retval->item=QDomElement();
   }
  return retval;
}

struct syntaxContextData* SyntaxDocument::getGroupInfo(const QString& mainGroupName, const QString &group)
{
  QDomElement docElem = documentElement();
  QDomNode n = docElem.firstChild();
  while (!n.isNull())
    {
      kdDebug(13010)<<"in SyntaxDocument::getGroupInfo (outer loop) " <<endl;
      QDomElement e=n.toElement();
      if (e.tagName().compare(mainGroupName)==0 )
        {
          QDomNode n1=e.firstChild();
          while (!n1.isNull())
            {
      	      kdDebug(13010)<<"in SyntaxDocument::getGroupInfo (inner loop) " <<endl;
              QDomElement e1=n1.toElement();
              if (e1.tagName()==group+"s")
                {
                 struct syntaxContextData *data=new (struct syntaxContextData);
                 data->parent=e1;
                 return data;
                }
              n1=e1.nextSibling();
            }
            kdDebug(13010) << "WARNING :returning null " << k_lineinfo << endl;
          return 0;
        }
      n=e.nextSibling();
    }
    kdDebug(13010) << "WARNING :returning null " << k_lineinfo << endl;
  return 0;
}


QStringList& SyntaxDocument::finddata(const QString& mainGroup,const QString& type,bool clearList)
{
  QDomElement e  = documentElement();
  if (clearList) m_data.clear();
//  if ( n.isElement())
    {
	for(QDomNode n=e.firstChild(); !n.isNull(); n=n.nextSibling())
	  {
            if (n.toElement().tagName()==mainGroup)
              {
                for (n=n.firstChild(); !n.isNull(); n=n.nextSibling())
		  {
                    if (n.toElement().tagName()==type)
                     {
		       QDomNodeList childlist=n.childNodes();
                       for (uint i=0; i<childlist.count();i++)
                         m_data+=childlist.item(i).toElement().text().stripWhiteSpace();
                       break;
                     }
                  }
                break;
              }
          }      
   }
  return m_data;
}
