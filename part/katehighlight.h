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

class HlContext;
class HlItem;
class ItemData;
class HlData;
class EmbeddedHlInfo;
class IncludeRule;
class SyntaxDocument;
class TextLine;
struct syntaxModeListItem;
struct syntaxContextData;

class QPopupMenu;

// some typedefs
typedef QPtrList<KateAttribute> KateAttributeList;
typedef QValueList<IncludeRule*> IncludeRules;
typedef QPtrList<ItemData> ItemDataList;
typedef QPtrList<HlData> HlDataList;
typedef QMap<QString,EmbeddedHlInfo> EmbeddedHlInfos;
typedef QMap<int*,QString> UnresolvedContextReferences;

//Item Properties: name, Item Style, Item Font
class ItemData : public KateAttribute
{
  public:
    ItemData(const QString  name, int defStyleNum);
    
  public:
    const QString name;
    int defStyleNum;
};

class HlData
{
  public:
    HlData(const QString &wildcards, const QString &mimetypes,const QString &identifier, int priority);
    
  public:
    QString wildcards;
    QString mimetypes;
    QString identifier;
    int priority;
};

class Highlight
{
  public:
    Highlight(const syntaxModeListItem *def);
    ~Highlight();

  public:
    void doHighlight(QMemArray<short> oCtx, TextLine *,bool lineContinue,QMemArray<signed char> *foldingList);

    void loadWildcards();
    QValueList<QRegExp>& getRegexpExtensions();
    QStringList& getPlainExtensions();
    
    QString getMimetypes();
    
    // this pointer needs to be deleted !!!!!!!!!!
    HlData *getData();
    void setData(HlData *);

    void setItemDataList(uint schema, ItemDataList &);
    
    // both methodes return hard copies of the internal lists
    // the lists are cleared first + autodelete is set !
    // keep track that you delete them, or mem will be lost
    void getItemDataListCopy (uint schema, ItemDataList &);
    
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
    // make this private, nobody should play with the internal data pointers
    void getItemDataList(uint schema, ItemDataList &);
  
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
    
    QString extensionSource;
    QValueList<QRegExp> regexpExtensions;
    QStringList plainExtensions;

  public:
    inline bool foldingIndentationSensitive () { return m_foldingIndentationSensitive; }
    inline bool allowsFolding(){return folding;}
};

class HlManager : public QObject
{
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

  private:
    int wildcardFind(const QString &fileName);
    int mimeFind(const QByteArray &contents, const QString &fname);
    int realWildcardFind(const QString &fileName);

  private:
    friend class Highlight;
    
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

// kate: space-indent on; indent-width 2; replace-tabs on;
