/* This file is part of the KDE libraries
   Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>

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

#ifndef _KATEINDENTSCRIPTABSTRACTS_H_
#define _KATEINDENTSCRIPTABSTRACTS_H_

#include <qstring.h>
#include <kdebug.h>

class KateView;
class KateDocCursor;

class KateIndentScriptManagerAbstract;

class KateIndentScriptImplAbstract {
  public:
    friend class KateIndentScript;
    KateIndentScriptImplAbstract(KateIndentScriptManagerAbstract *manager, const QString& internalName,
        const QString  &filePath, const QString &niceName,
        const QString &license,bool hasCopyright, double version);
    virtual ~KateIndentScriptImplAbstract();
    
    virtual bool processChar( KateView *view, QChar c, QString &errorMsg )=0;
    virtual bool processLine( KateView *view, const KateDocCursor &line, QString &errorMsg )=0;
    virtual bool processNewline( KateView *view, const KateDocCursor &begin, bool needcontinue, QString &errorMsg )=0;
  public:
    QString internalName();
    QString filePath();
    QString niceName();
    QString license();
    QString copyright();
    double version();
  protected:
    virtual void decRef();
    long refCount() {return m_refcount;}
    QString filePath() const {return m_filePath;}
  private:
    void incRef();
    long m_refcount;
    KateIndentScriptManagerAbstract *m_manager;
    QString m_internalName;
    QString m_filePath;
    QString m_niceName;
    QString m_license;
    bool m_hasCopyright;
    double m_version;
};


class KateIndentScript {
  public:
    KateIndentScript(KateIndentScriptImplAbstract *scr):m_scr(scr) { }
    ~KateIndentScript() {}
    KateIndentScript():m_scr(0) {}
    KateIndentScript(const KateIndentScript &p):m_scr(p.m_scr){}
    KateIndentScript &operator=(const KateIndentScript &p) {
      if (m_scr==p.m_scr) return *this;
      m_scr=p.m_scr;
      return *this;
    }
    /*operator KateIndentJScript*() const { return m_scr; }*/
    bool processChar( KateView *view, QChar c, QString &errorMsg ) {
      kDebug(13050)<<"KateIndentScript::processChar: m_scr:"<<m_scr<<endl;
      if (m_scr) return m_scr->processChar(view,c,errorMsg); else return true;
    }
    bool processLine( KateView *view, const KateDocCursor& line, QString &errorMsg ) {
      kDebug(13050)<<"KateIndentScript::processLine: m_scr:"<<m_scr<<endl;
      if (m_scr) return m_scr->processLine(view,line,errorMsg); else return true;
    }
    bool processNewline( KateView *view, const KateDocCursor& begin, bool needcontinue, QString &errorMsg ) {
      kDebug(13050)<<"KateIndentScript::processNewLine: m_scr:"<<m_scr<<endl;
      if (m_scr) return m_scr->processNewline(view,begin,needcontinue,errorMsg); else return true;
    }

    QString internalName() {if (m_scr) return m_scr->internalName(); else return QString();}
    QString filePath() {if (m_scr) return m_scr->filePath(); else return QString();}
    QString niceName() {if (m_scr) return m_scr->niceName(); else return QString();}
    QString license() {if (m_scr) return m_scr->license(); else return QString();}
    QString copyright() {if (m_scr) return m_scr->copyright(); else return QString();}
    double version() {if (m_scr) return m_scr->version(); else return -1;}

    bool isNull () const {return (m_scr==0);}
  private:
    KateIndentScriptImplAbstract *m_scr;
};

class KateIndentScriptManagerAbstract
{

  public:
    KateIndentScriptManagerAbstract () {}
    virtual ~KateIndentScriptManagerAbstract () {}
    virtual KateIndentScript script(const QString &scriptname)=0;
    virtual QString copyright(KateIndentScriptImplAbstract* script)=0;
};

#endif
