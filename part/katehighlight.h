/* This file is part of the KDE libraries
   Copyright (C) 2001,2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef _KATE_HIGHLIGHT_H_
#define _KATE_HIGHLIGHT_H_

#include "katetextline.h"
#include "kateattribute.h"

#include "../interfaces/document.h"

#include <kconfig.h>

#include <qptrlist.h>
#include <qvaluelist.h>
#include <qregexp.h>
#include <qdict.h>
#include <qintdict.h>
#include <qmap.h>
#include <qobject.h>
#include <qstringlist.h>
#include <qguardedptr.h>

class SyntaxDocument;
class TextLine;
struct syntaxModeListItem;
struct syntaxContextData;

class QPopupMenu;

class HlItem {
  public:
    HlItem(int attribute, int context,signed char regionId, signed char regionId2);
    virtual ~HlItem();
    virtual bool alwaysStartEnable() const { return true; };
    virtual bool hasCustomStartEnable() const { return false; };
    virtual bool startEnable(const QChar&);

    // Changed from using QChar*, because it makes the regular expression check very
    // inefficient (forces it to copy the string, very bad for long strings)
    // Now, the function returns the offset detected, or 0 if no match is found.
    // bool linestart isn't needed, this is equivalent to offset == 0.
    virtual int checkHgl(const QString& text, int offset, int len) = 0;

    virtual bool lineContinue(){return false;}

    QPtrList<HlItem> *subItems;
    int attr;
    int ctx;
    signed char region;
    signed char region2;
};

typedef QPtrList<KateAttribute> KateAttributeList;

class IncludeRule {
  public:
    IncludeRule(int ctx_, uint pos_, const QString &incCtxN_) {ctx=ctx_;pos=pos_;incCtxN=incCtxN_;incCtx=-1;}
    IncludeRule(int ctx_, uint  pos_) {ctx=ctx_;pos=pos_;incCtx=-1;incCtxN="";}
    uint pos;
    int ctx;
    int incCtx;
    QString incCtxN;
};

typedef QValueList<IncludeRule*> IncludeRules;

//Item Properties: name, Item Style, Item Font
class ItemData : public KateAttribute {
  public:
    ItemData(const QString  name, int defStyleNum);
    const QString name;
    int defStyleNum;
};

typedef QPtrList<ItemData> ItemDataList;

class HlData {
  public:
    HlData(const QString &wildcards, const QString &mimetypes,const QString &identifier, int priority);
    QString wildcards;
    QString mimetypes;
    QString identifier;
    int priority;
};

typedef QPtrList<HlData> HlDataList;

//context
class HlContext {
  public:
    HlContext (int attribute, int lineEndContext,int _lineBeginContext,
               bool _fallthrough, int _fallthroughContext);

    QPtrList<HlItem> items;
    int attr;
    int ctx;
    int lineBeginContext;
    /** @internal anders: possible escape if no rules matches.
       false unless 'fallthrough="1|true"' (insensitive)
       if true, go to ftcxt w/o eating of string.
       ftctx is "fallthroughContext" in xml files, valid values are int or #pop[..]
       see in Highlight::doHighlight */
    bool fallthrough;
    int ftctx; // where to go after no rules matched
};


class EmbeddedHlInfo
{
public:
  EmbeddedHlInfo() {loaded=false;context0=-1;}
  EmbeddedHlInfo(bool l, int ctx0) {loaded=l;context0=ctx0;}
  bool loaded;
  int context0;
};

typedef QMap<QString,EmbeddedHlInfo> EmbeddedHlInfos;

typedef QMap<int*,QString> UnresolvedContextReferences; // need to be made more efficient, but it works for the moment

class Highlight
{
  public:
    Highlight(const syntaxModeListItem *def);
    ~Highlight();

    void doHighlight(QMemArray<short> oCtx, TextLine *,bool lineContinue,QMemArray<signed char> *foldingList);

    QString getWildcards();
    QString getMimetypes();
    
    // this pointer needs to be deleted !!!!!!!!!!
    HlData *getData();
    void setData(HlData *);

    void getItemDataList(uint schema, ItemDataList &);
    void setItemDataList(uint schema, ItemDataList &);
    
    inline QString name() const {return iName;}
    inline QString section() const {return iSection;}
    inline QString version() const {return iVersion;}
    int priority();
    inline QString getIdentifier() const {return identifier;}
    void use();
    void release();
    bool isInWord(QChar c);

    inline QString getCommentStart() const {return cmlStart;};
    inline QString getCommentEnd()  const {return cmlEnd;};
    inline QString getCommentSingleLineStart() const { return cslStart;};
    
    void clearAttributeArrays ();
    
    QMemArray<KateAttribute> *attributes (uint schema);

  private:
    void init();
    void done();
    void makeContextList ();
    void handleIncludeRules ();
    void handleIncludeRulesRecursive(IncludeRules::iterator it, IncludeRules *list);
    int addToContextList(const QString &ident, int ctx0);
    void addToItemDataList();
    void createItemData (ItemDataList &list);
    void readGlobalKeywordConfig();
    void readCommentConfig();
    void readFoldingConfig ();

    // manipulates the ctxs array directly ;)
    void generateContextStack(int *ctxNum, int ctx, QMemArray<short> *ctxs, int *posPrevLine,bool lineContinue=false);

    HlItem *createHlItem(struct syntaxContextData *data, ItemDataList &iDl, QStringList *RegionList, QStringList *ContextList);
    int lookupAttrName(const QString& name, ItemDataList &iDl);

    void createContextNameList(QStringList *ContextNameList, int ctx0);
    int getIdFromString(QStringList *ContextNameList, QString tmpLineEndContext,/*NO CONST*/ QString &unres);

    ItemDataList internalIDList;

    QIntDict<HlContext> contextList;
    HlContext *contextNum (uint n);

    // make them pointers perhaps
    EmbeddedHlInfos embeddedHls;
    UnresolvedContextReferences unresolvedContextReferences;
    QStringList RegionList;
    QStringList ContextNameList;

    bool noHl;
    bool folding;
    bool casesensitive;
    QString weakDeliminator;
    QString deliminator;
    //const QChar *deliminatorChars;
    //uint deliminatorLen;
    QString cmlStart;
    QString cmlEnd;
    QString cslStart;
    QString iName;
    QString iSection;
    QString iWildcards;
    QString iMimetypes;
    QString identifier;
    QString iVersion;
    int m_priority;
    int refCount;

    QString errorsAndWarnings;
    QString buildIdentifier;
    QString buildPrefix;
    bool building;
    uint itemData0;
    uint buildContext0Offset;
    IncludeRules includeRules;
    QValueList<int> contextsIncludingSomething;
    bool m_foldingIndentationSensitive;
    
    QIntDict< QMemArray<KateAttribute> > m_attributeArrays;

    public:
      inline bool foldingIndentationSensitive () { return m_foldingIndentationSensitive; }
      inline bool allowsFolding(){return folding;}
};

class HlManager : public QObject
{
  friend class Highlight;

  Q_OBJECT
  
  private:
    HlManager();
    
  public:
    ~HlManager();

    static HlManager *self();
    
    inline KConfig *getKConfig() { return &m_config; };
    
    Highlight *getHl(int n);
    int nameFind(const QString &name);

    int detectHighlighting (class KateDocument *doc);

    int findHl(Highlight *h) {return hlList.find(h);}
    QString identifierForName(const QString&);

    // methodes to get the default style count + names
    static uint defaultStyles();
    static QString defaultStyleName(int n);
    
    void getDefaults(uint schema, KateAttributeList &);
    void setDefaults(uint schema, KateAttributeList &);

    int highlights();
    QString hlName(int n);
    QString hlSection(int n);

  signals:
    void changed();

    // A highlight hlNumber changes the hlData.
    void changed( uint hlNumber );

  public:
    void emitChanged() { emit changed(); }; // ehm, needed atm. I'll try making something nicer. Anders.
    void emitChanged(uint n) { emit changed( n ); };

  private:
    int wildcardFind(const QString &fileName);
    int mimeFind(const QByteArray &contents, const QString &fname);
    int realWildcardFind(const QString &fileName);

  private:
    friend class HlEditDialog;
  
    QPtrList<Highlight> hlList;
    QDict<Highlight> hlDict;

    static HlManager *s_self;
    
    KConfig m_config;
    QStringList commonSuffixes;
    
    SyntaxDocument *syntax;
};

class KateViewHighlightAction: public Kate::ActionMenu
{
  Q_OBJECT

  public:
    KateViewHighlightAction(const QString& text, QObject* parent = 0, const char* name = 0)
       : Kate::ActionMenu(text, parent, name) { init(); };

    ~KateViewHighlightAction(){;};

    void updateMenu (Kate::Document *doc);

  private:
    void init();

    QGuardedPtr<Kate::Document> m_doc;
    QStringList subMenusName;
    QStringList names;
    QPtrList<QPopupMenu> subMenus;

  public  slots:
    void slotAboutToShow();

  private slots:
    void setHl (int mode);
};


#endif //_HIGHLIGHT_H_
