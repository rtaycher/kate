/*
   Copyright (C) 1998, 1999 Jochen Wilhelmy
                            digisnap@cs.tu-berlin.de

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

#ifndef _HIGHLIGHT_H_
#define _HIGHLIGHT_H_

#include <qlist.h>
#include <qdialog.h>

#include <kcolorbtn.h>
#include <qstrvec.h>
#include <qdict.h>
#include <qregexp.h>
#include "../qt3back/qregexp3.h"
#include <kdebug.h>

class SyntaxDocument;
struct syntaxModeListItem;
struct syntaxContextData;

class QCheckBox;
class QComboBox;
class QLineEdit;

class TextLine;
class Attribute;


bool isInWord(QChar); //true for '_','0'-'9','A'-'Z','a'-'z'

class HlItem {
  public:
    HlItem(int attribute, int context);
    virtual ~HlItem();
    virtual bool startEnable(QChar) {return true;}
    virtual bool endEnable(QChar) {return true;}
    virtual const QChar *checkHgl(const QChar *,bool) = 0;
    QList<HlItem> *subItems;
    int attr;
    int ctx;    
};

class HlItemWw : public HlItem {
  public:
    HlItemWw(int attribute, int context);
    virtual bool startEnable(QChar c) {return !isInWord(c);}
    virtual bool endEnable(QChar c) {return !isInWord(c);}
};


class HlCharDetect : public HlItem {
  public:
    HlCharDetect(int attribute, int context, QChar);
    virtual const QChar *checkHgl(const QChar *,bool);
  protected:
    QChar sChar;
};

class Hl2CharDetect : public HlItem {
  public:
    Hl2CharDetect(int attribute, int context,  QChar ch1, QChar ch2);
   	Hl2CharDetect(int attribute, int context, const QChar *ch);

    virtual const QChar *checkHgl(const QChar *,bool);
  protected:
    QChar sChar1;
    QChar sChar2;
};

class HlStringDetect : public HlItem {
  public:
    HlStringDetect(int attribute, int context, const QString &, bool inSensitive=false);
    virtual ~HlStringDetect();
    virtual const QChar *checkHgl(const QChar *,bool);
  protected:
    const QString str;
    bool _inSensitive;
};

class HlRangeDetect : public HlItem {
  public:
    HlRangeDetect(int attribute, int context, QChar ch1, QChar ch2);
    virtual const QChar *checkHgl(const QChar *,bool);
  protected:
    QChar sChar1;
    QChar sChar2;
};

/*
class KeywordData {
  public:
    KeywordData(const QString &);
    ~KeywordData();
    char *s;
    int len;
};
*/
class HlKeyword : public HlItemWw {
  public:
    HlKeyword(int attribute, int context,bool casesensitive,QString weakSep);
    virtual ~HlKeyword();
    virtual void addWord(const QString &);
		virtual void addList(const QStringList &);
// needed for kdevelop (if they decide to use this code)
    virtual void addList(const char **);
    virtual const QChar *checkHgl(const QChar *,bool);
		QStringList getList() { return words;};
		QDict<char> getDict() { return Dict;};

  protected:
    QStringList words;
    QDict<char> Dict;
    bool _caseSensitive;
    QString _weakSep;
    const QChar*  (*doCheckHgl)(const QChar* ,bool,HlKeyword*);
    static const QChar* sensitiveCheckHgl(const QChar*,bool,HlKeyword *kw);
    static const QChar *inSensitiveCheckHgl(const QChar *s,bool,HlKeyword *kw);
};

#if 0
class HlCaseInsensitiveKeyword : public HlKeyword {
  public:
    HlCaseInsensitiveKeyword(int attribute, int context);
    virtual ~HlCaseInsensitiveKeyword();
    virtual const char *checkHgl(const char *,bool);
		virtual const QChar *checkHgl(const QChar *,bool);
    void addList(const QStringList &);
    void addList(const char **);
};

#endif

class HlPHex : public HlItemWw {
  public:
    HlPHex(int attribute,int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};
class HlInt : public HlItemWw {
  public:
    HlInt(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlFloat : public HlItemWw {
  public:
    HlFloat(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlCInt : public HlInt {
  public:
    HlCInt(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlCOct : public HlItemWw {
  public:
    HlCOct(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlCHex : public HlItemWw {
  public:
    HlCHex(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlCFloat : public HlFloat {
  public:
    HlCFloat(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlLineContinue : public HlItem {
  public:
    HlLineContinue(int attribute, int context);
    virtual bool endEnable(QChar c) {return c == '\0';}
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlCStringChar : public HlItem {
  public:
    HlCStringChar(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlCChar : public HlItemWw {
  public:
    HlCChar(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};







class HlAnyChar : public HlItem {
  public:
    HlAnyChar(int attribute, int context,char* charList);
    virtual const QChar *checkHgl(const QChar *,bool);
    char* _charList;
};

class HlRegExpr : public HlItem {
  public:
  HlRegExpr(int attribute, int context,QString expr);
  ~HlRegExpr(){delete Expr;};
  virtual const QChar *checkHgl(const QChar *,bool);
  QRegExp3 *Expr;
  bool handlesLinestart;
};

//--------


//Item Style: color, selected color, bold, italic
class ItemStyle {
  public:
    ItemStyle();
//    ItemStyle(const ItemStyle &);
    ItemStyle(const QColor &, const QColor &, bool bold, bool italic);
    ItemStyle(ItemStyle *its){col=its->col;selCol=its->selCol; bold=its->bold; italic=its->italic;}
//    void setData(const ItemStyle &);
    QColor col;
    QColor selCol;
    int bold;   //boolean value
    int italic; //boolean value
};

typedef QList<ItemStyle> ItemStyleList;

//Item Properties: name, Item Style, Item Font
class ItemData : public ItemStyle {
  public:
    ItemData(const QString  name, int defStyleNum);
    ItemData(const QString  name, int defStyleNum,
      const QColor&, const QColor&, bool bold, bool italic);
    ItemData(ItemData 
*itd):ItemStyle((ItemStyle*)itd),name(itd->name),defStyleNum(itd->defStyleNum),defStyle(itd->defStyle){;}
    const QString name;
    int defStyleNum;
    int defStyle; //boolean value
};

typedef QList<ItemData> ItemDataList;

class HlData {
  public:
    HlData(const QString &wildcards, const QString &mimetypes,const QString &identifier);
    ItemDataList itemDataList;
    QString wildcards;
    QString mimetypes;
    QString identifier;
};

typedef QList<HlData> HlDataList;

class HlManager;
class KConfig;

class Highlight {
    friend class HlManager;
  public:
    Highlight(const char * name);
    virtual ~Highlight();
    KConfig *getKConfig();
    QString getWildcards();
    QString getMimetypes();
    HlData *getData();
    void setData(HlData *);
    void getItemDataList(ItemDataList &);
    virtual void getItemDataList(ItemDataList &, KConfig *);
    virtual void setItemDataList(ItemDataList &, KConfig *);
    const char * name() {return iName;}
//    QString extensions();
//    QString mimetypes();
    void use();
    void release();
    virtual bool isInWord(QChar c) {return ::isInWord(c);}
    virtual int doHighlight(int ctxNum, TextLine *textLine);
    virtual QString getCommentStart() { return QString(""); };
    virtual QString getCommentEnd() { return QString(""); };
    virtual QString getCommentSingleLineStart() { return QString("");};
  protected:
    virtual void createItemData(ItemDataList &);
    virtual void init();
    virtual void done();
    const char * iName;
    QString iWildcards;
    QString iMimetypes;
    QString identifier;
    int refCount;
};


//context
class HlContext {
  public:
    HlContext(int attribute, int lineEndContext);
    QList<HlItem> items;
    int attr;
    int ctx;
};

class GenHighlight : public Highlight {
  public:
    GenHighlight(const char * name);

    virtual int doHighlight(int ctxNum, TextLine *);
  protected:
    virtual void makeContextList() = 0;
    virtual void init();
    virtual void done();

    static const int nContexts = 32;
    HlContext *contextList[nContexts];
};


class AutoHighlight : public GenHighlight
{
  public:
    AutoHighlight(syntaxModeListItem *def);
    virtual ~AutoHighlight();
    QString getCommentStart() {kdDebug()<<"*****AutoHighlight::getCommentStart()"<<endl; return cmlStart;};
    QString getCommentEnd()  {return cmlEnd;};
    QString getCommentSingleLineStart() { return cslStart;};

  protected:
    QString iName;
    QString casesensitive;
    QString cmlStart;
    QString cmlEnd;
    QString cslStart;
    virtual void makeContextList ();
    virtual void setKeywords (HlKeyword *keyword,HlKeyword *dataType);
    virtual void createItemData (ItemDataList &list);
    HlItem *createHlItem(struct syntaxContextData *data, int *res);
    ItemDataList internalIDList;
};

//class KWriteDoc;

class HlManager : public QObject {
    Q_OBJECT
  public:
    HlManager();
    ~HlManager();

    static HlManager *self();

    Highlight *getHl(int n);
    int defaultHl();
    int nameFind(const QString &name);

    int wildcardFind(const QString &fileName);
    int mimeFind(const QByteArray &contents, const QString &fname);
    int findHl(Highlight *h) {return hlList.find(h);}

    int makeAttribs(Highlight *, Attribute *, int maxAttribs);

    int defaultStyles();
    const char * defaultStyleName(int n);
    void getDefaults(ItemStyleList &);
    void setDefaults(ItemStyleList &);

    int highlights();
    const char * hlName(int n);
    void getHlDataList(HlDataList &);
    void setHlDataList(HlDataList &);

    SyntaxDocument *syntax;

  signals:
    void changed();
  protected:
    QList<Highlight> hlList;
    static HlManager *s_pSelf;
};




#endif //_HIGHLIGHT_H_
