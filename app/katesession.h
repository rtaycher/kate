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
#include <ksimpleconfig.h>

#include <qobject.h>
#include <qvaluelist.h>

class KateSessionManager;

class KDirWatch;
class KListView;

class QCheckBox;

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
     * relative session filename
     * @return relative filename for this session
     */
    const QString &sessionFileRelative () const { return m_sessionFileRel; }

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

    /**
     * config to read
     * YOU MUST DELETE THE POINTER
     * @return config to read from
     */
    KConfig *configRead ();

    /**
     * config to write
     * YOU MUST DELETE THE POINTER
     * @return config to write from
     */
    KConfig *configWrite ();

    /**
     * count of documents in this session
     * @return documents count
     */
    unsigned int documents () const { return m_documents; }

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
     * number of document of this session
     */
    unsigned int m_documents;

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
     * @param session session to activate
     * @param closeLast try to close last session or not?
     * @param saveLast try to save last session or not?
     * @param loadNew load new session stuff?
     */
    void activateSession (const KateSession &session, bool closeLast = true, bool saveLast = true, bool loadNew = true);

    /**
     * create a new session
     * @param name session name
     */
    KateSession createSession (const QString &name);

    /**
     * save current session
     * for sessions without filename: save nothing
     */
    bool saveActiveSession ();

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

    /**
     * initial session chooser, on app start
     */
    void chooseSession ();

  public slots:
    /**
     * try to start a new session
     * asks user first for name
     */
    void sessionNew ();

    /**
     * try to open a existing session
     */
    void sessionOpen ();

    /**
     * try to save current session
     */
    void sessionSave ();

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
    KateSessionChooser (QWidget *parent, const QString &lastSession, bool reopenLast);
    ~KateSessionChooser ();

    KateSession *selectedSession ();

    bool reopenLastSession ();

    enum {
      resultOpen,
      resultNew,
      resultNone
    };

  protected slots:
    /**
     * open session
     */
    void slotUser1 ();

    /**
     * new session
     */
    void slotUser2 ();

  private:
    KListView *m_sessions;
    QCheckBox *m_useLast;
};

class KateSessionOpenDialog : public KDialogBase
{
  Q_OBJECT

  public:
    KateSessionOpenDialog (QWidget *parent);
    ~KateSessionOpenDialog ();

    KateSession *selectedSession ();

    enum {
      resultOk,
      resultCancel
    };

  protected slots:
    /**
     * ok pressed
     */
    void slotUser1 ();

    /**
     * cancel pressed
     */
    void slotUser2 ();

  private:
    KListView *m_sessions;
};

#endif
