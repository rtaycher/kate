#include "kateviewhighlightaction.h"
#include "kateviewhighlightaction.moc"
#include "kateview.h"
#include "katehighlight.h"
#include <kpopupmenu.h>
#include "../interfaces/viewmanager.h"

void KateViewHighlightAction::init(QObject *related_)
{
	related=related_;
	connect(popupMenu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateViewHighlightAction::slotAboutToShow()
{
  kdDebug()<<"KateViewHighlightAction::slotAboutToShow()"<<endl;

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
      if (dynamic_cast<KateView*>(related))
         subMenus.at(m)->insertItem ( hlName, related, SLOT(setHl(int)), 0,  z);
      else
	{
	  if (dynamic_cast<Kate::ViewManager*>(related))
            subMenus.at(m)->insertItem ( hlName, related, SLOT(slotSetHl(int)), 0,  z);
		else kdDebug()<<"Ohoh, related must be a Kate::ViewManager or a KateView"<<endl;
	}
    }
    else if (names.contains(hlName) < 1)
    {
      names << hlName;
      if (dynamic_cast<KateView*>(related))
        popupMenu()->insertItem ( hlName, related, SLOT(setHl(int)), 0,  z);
      else
	{
	  if (dynamic_cast<Kate::ViewManager*>(related))
            popupMenu()->insertItem ( hlName, related, SLOT(slotSetHl(int)), 0,  z);
		else kdDebug()<<"Ohoh, related must be a Kate::ViewManager or a KateView"<<endl;
	}
    }
  }

  Kate::ViewManager *viewManager=dynamic_cast<Kate::ViewManager*>(related);
  KateView *view=dynamic_cast<KateView*>(related);
  if ((!view) && (!viewManager)) return;

  if (view?true:(bool)viewManager->getActiveView())
  {
    for (int i=0;i<subMenus.count();i++)
    {
      for (int i2=0;i2<subMenus.at(i)->count();i2++)
      	subMenus.at(i)->setItemChecked(subMenus.at(i)->idAt(i2),false);
    }
    popupMenu()->setItemChecked (0, false);

    int i = subMenusName.findIndex (HlManager::self()->hlSection
	(view?view->getHl():viewManager->getActiveView()->getHl()));
    if (subMenus.at(i))
      subMenus.at(i)->setItemChecked (
	(view?view->getHl():viewManager->getActiveView()->getHl()), true);
    else
      popupMenu()->setItemChecked (0, true);

    oldActiveSec = HlManager::self()->hlSection (
	view?view->getHl():viewManager->getActiveView()->getHl());
    oldActiveID = (view?view->getHl():viewManager->getActiveView()->getHl());
  }

}
