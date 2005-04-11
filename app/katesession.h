/* This file is part of the KDE project
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __KATE_SESSION_H__
#define __KATE_SESSION_H__

#include "katemain.h"

#include <kdialogbase.h>

#include <qobject.h>
#include <qvaluelist.h>

class KateSessionManager;

class KDirWatch;
class KListView;

class KateSession
{
  public:
    /**
     * create a session from given file
     * @param fileName session filename, relative
     * @param name session name
     * @param manager pointer to the manager
     */
    KateSession (KateSessionManager *manager, const QString &fileName, const QString &name);

    /**
     * destruct me
     */
    ~KateSession ();

    /**
     * session filename, absolute, calculated out of relative filename + session dir
     * @return absolute path to session file
     */
    QString sessionFile () const;

    /**
     * session name
     * @return name for this session
     */
    const QString &sessionName () const { return m_sessionName; }

    /**
     * is this a valid session? if not, don't use any session if this is
     * the active one
     */
    bool isValid () const { return !(m_sessionFileRel.isEmpty() || m_sessionName.isEmpty()); }

  private:
    /**
     * session filename, in local location we can write to
     * relative filename to the session dirs :)
     */
    QString m_sessionFileRel;

    /**
     * session name, extracted from the file, to display to the user
     */
    QString m_sessionName;

    /**
     * KateSessionMananger
     */
    KateSessionManager *m_manager;
};

typedef QValueList<KateSession *> KateSessionList;

class KateSessionManager : public QObject
{
  Q_OBJECT

  public:
    KateSessionManager(QObject *parent);
    ~KateSessionManager();

    /**
     * allow access to this :)
     * @return instance of the session manager
     */
    static KateSessionManager *self();

    /**
     * allow access to the session list
     * kept up to date by watching the dir
     */
    inline KateSessionList & sessionList () { return m_sessionList; }

    /**
     * activate a session
     * first, it will look if a session with this name exists in list
     * if yes, it will use this session, else it will create a new session file
     * @param name session name to activate
     */
    void activateSession (const QString &name);

    /**
     * return the current active session
     * sessionFile == empty means we have no session around for this instance of kate
     * @return session active atm
     */
    inline KateSession & activeSession () { return m_activeSession; }

    /**
     * session dir
     * @return global session dir
     */
    inline const QString &sessionsDir () const { return m_sessionsDir; }

  private slots:
    void dirty (const QString &path);

  private:
    void updateSessionList ();

  private:
    /**
     * absolute path to dir in home dir where to store the sessions
     */
    QString m_sessionsDir;

    /**
     * dirwatch object to keep track of this dir
     */
    KDirWatch *m_dirWatch;

    /**
     * list of current available sessions
     */
    KateSessionList m_sessionList;

    /**
     * current active session
     */
    KateSession m_activeSession;
};

class KateSessionChooser : public KDialogBase
{
  Q_OBJECT

  public:
    KateSessionChooser (QWidget *parent = 0);
    ~KateSessionChooser ();

  private:
    KListView *m_sessions;
};

#endif
