#ifndef _KATEVIEW_HIGHLIGHTACTION_H_
#define _KATEVIEW_HIGHLIGHTACTION_H_
#include <kaction.h>
#include <qstringlist.h>
#if QT_VERSION < 300
#include <qlist.h>
#else
#include <qptrlist.h>
#endif
#include <qpopupmenu.h>

class KateViewHighlightAction: public KActionMenu
{
	Q_OBJECT
public:
	KateViewHighlightAction( QObject *related, const QString& text, QObject* parent = 0,
		const char* name = 0):KActionMenu(text, parent ,
		name){init(related);}

	KateViewHighlightAction( QObject *related, const QString& text, const QIconSet& icon,
		 QObject* parent = 0, const char* name = 0 )
		: KActionMenu(text, icon, parent, name){init(related);}

	KateViewHighlightAction( QObject *related,const QString& text, const QString& icon,
                QObject* parent = 0, const char* name = 0 )
		:KActionMenu(text,icon, parent, name ){init(related);}

	KateViewHighlightAction( QObject* related, QObject* parent = 0, const char* name = 0 )
    		:KActionMenu( parent, name) {init(related);}

	~KateViewHighlightAction(){;}
private:
	QObject *related;
	void init(QObject *related_);
	QStringList subMenusName;
	QStringList names;
#if QT_VERSION < 300
        QList<QPopupMenu> subMenus;
#else
        QPtrList<QPopupMenu> subMenus;
#endif
public  slots:
	void slotAboutToShow();
};

#endif
