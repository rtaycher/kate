/***************************************************************************
                          katehighlightaction.cpp  -  description
                             -------------------
    begin                : Sat 31 March 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "kateviewhighlightaction.h"
#include "kateviewhighlightaction.moc"
#include "kateview.h"
#include "katedocument.h"
#include "katehighlight.h"
#include <kpopupmenu.h>
#include "../interfaces/viewmanager.h"
#include "../interfaces/currentdocprovider.h"

void KateViewHighlightAction::init(QObject *related_)
{
	subMenus.setAutoDelete( true );
	related=related_;
	connect(popupMenu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateViewHighlightAction::slotAboutToShow()
{
  kdDebug()<<"KateViewHighlightAction::slotAboutToShow()"<<endl;

  QObject *_related=related;

  int count = HlManager::self()->highlights();
  static QString oldActiveSec;
  static int oldActiveID;

  for (int z=0; z<count; z++)
  {
    QString hlName = HlManager::self()->hlName (z);
    QString hlSection = HlManager::self()->hlSection (z);

    if ((hlSection != "") && (names.contains(hlName) < 1))
    {
      if (subMenusName.contains(hlSection) < 1)
      {
        subMenusName << hlSection;
        QPopupMenu *menu = new QPopupMenu ();
        subMenus.append(menu);
        popupMenu()->insertItem (hlSection, menu);
      }

      int m = subMenusName.findIndex (hlSection);
      names << hlName;
      subMenus.at(m)->insertItem ( hlName, this, SLOT(setHl(int)), 0,  z);
    }
    else if (names.contains(hlName) < 1)
    {
      names << hlName;
      popupMenu()->insertItem ( hlName, this, SLOT(setHl(int)), 0,  z);
    }
  }

  Kate::KateCurrentDocProvider *docProvider=dynamic_cast<Kate::KateCurrentDocProvider*>(_related);
  KateView *view=dynamic_cast<KateView*>(_related);

  if (view) kdDebug()<<"KateViewHighlightAction: related is a View"<<endl;
  if (docProvider) kdDebug()<<"KateViewHighlightAction: related is a KateCurrentDocProvider"<<endl;

  if ((!view) && (!docProvider)) return;

  if (view?true:(bool)docProvider->getCurrentDoc())
  {
    for (uint i=0;i<subMenus.count();i++)
    {
      for (uint i2=0;i2<subMenus.at(i)->count();i2++)
      	subMenus.at(i)->setItemChecked(subMenus.at(i)->idAt(i2),false);
    }
    popupMenu()->setItemChecked (0, false);

    int i = subMenusName.findIndex (HlManager::self()->hlSection
	(view?view->doc()->hlMode():docProvider->getCurrentDoc()->hlMode()));
    if (subMenus.at(i))
      subMenus.at(i)->setItemChecked ((view?view->doc()->hlMode():docProvider->getCurrentDoc()->hlMode()), true);
    else
      popupMenu()->setItemChecked (0, true);

    oldActiveSec = HlManager::self()->hlSection (
	view?view->doc()->hlMode():docProvider->getCurrentDoc()->hlMode());
    oldActiveID = (view?view->doc()->hlMode():docProvider->getCurrentDoc()->hlMode());
  }

}

void KateViewHighlightAction::setHl (int mode)
{
  QObject *_related=related;
  if (dynamic_cast<KateView*>(_related))
    dynamic_cast<KateView*>(_related)->doc()->setHlMode((uint)mode);
  else
	  if (dynamic_cast<Kate::KateCurrentDocProvider*>(_related))
      		dynamic_cast<Kate::KateCurrentDocProvider*>(_related)->getCurrentDoc()->setHlMode((uint)mode);

}
