/**
    This file is mostly copied from kio/kfile.
    No author was mentioned in the original.

    Slightly modified by Anders Lund.

    Fri Mar 29 01:26:26 CET 2002
    Copyright 2002, kfile authors and Anders Lund <anders@alweb.dk>

    Thanks to the kfile folks;)
*/


#ifndef _KBOOKMARKHANDLER_H_
#define _KBOOKMARKHANDLER_H_

#include <kbookmarkmanager.h>
#include <kbookmarkmenu.h>
#include "katefileselector.h"

class QTextStream;
class KPopupMenu;
class KActionMenu;

class KBookmarkHandler : public QObject, public KBookmarkOwner
{
    Q_OBJECT

public:
    KBookmarkHandler( KateFileSelector *parent, KPopupMenu *kpopupmenu=0 );
    ~KBookmarkHandler();

    // KBookmarkOwner interface:
    virtual void openBookmarkURL( const QString& url ) { emit openURL( url ); }
    virtual QString currentURL() const;

    KPopupMenu *menu() const { return m_menu; }

signals:
    void openURL( const QString& url );

private slots:
    void slotNewBookmark( const QString& text, const QCString& url,
                          const QString& additionalInfo );
    void slotNewFolder( const QString& text, bool open,
                        const QString& additionalInfo );
    void newSeparator();
    void endFolder();

protected:
    virtual void virtual_hook( int id, void* data );

private:
    KateFileSelector *mParent;
    KPopupMenu *m_menu;
    KBookmarkMenu *m_bookmarkMenu;

    QTextStream *m_importStream;

    //class KBookmarkHandlerPrivate *d;
};


#endif // _KBOOKMARKHANDLER_H_
