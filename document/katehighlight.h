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
    HlKeyword(int attribute, int context);
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
};
class HlCaseInsensitiveKeyword : public HlKeyword {
  public:
    HlCaseInsensitiveKeyword(int attribute, int context);
    virtual ~HlCaseInsensitiveKeyword();
    virtual const char *checkHgl(const char *,bool);
		virtual const QChar *checkHgl(const QChar *,bool);
    void addList(const QStringList &);
    void addList(const char **);
};

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

class HlCSymbol : public HlItem {
  public:
    HlCSymbol(int attribute, int context);
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

class HlCPrep : public HlItem {
  public:
    HlCPrep(int attribute, int context);
    virtual bool startEnable(QChar c) {return c == '\0';}
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlHtmlTag : public HlItem {
  public:
    HlHtmlTag(int attribute, int context);
    virtual bool startEnable(QChar c) {return c == '<';}
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlHtmlValue : public HlItem {
  public:
    HlHtmlValue(int attribute, int context);
    virtual bool startEnable(QChar c) {return c == '=';}
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlShellComment : public HlCharDetect {
  public:
    HlShellComment(int attribute, int context);
    virtual bool startEnable(QChar c) {return !isInWord(c);}
};

//modula 2 hex
class HlMHex : public HlItemWw {
  public:
    HlMHex(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};


//ada decimal
class HlAdaDec : public HlItemWw {
  public:
    HlAdaDec(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

//ada base n
class HlAdaBaseN : public HlItemWw {
  public:
    HlAdaBaseN(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

//ada float
class HlAdaFloat : public HlItemWw {
  public:
    HlAdaFloat(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

//ada character
class HlAdaChar : public HlItemWw {
  public:
    HlAdaChar(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlSatherClassname : public HlItemWw {
  public:
    HlSatherClassname(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlSatherIdent : public HlItemWw {
  public:
    HlSatherIdent(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlSatherDec : public HlItemWw {
  public:
    HlSatherDec(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlSatherBaseN : public HlItemWw {
  public:
    HlSatherBaseN(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlSatherFloat : public HlItemWw {
  public:
    HlSatherFloat(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlSatherChar : public HlItemWw {
  public:
    HlSatherChar(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlSatherString : public HlItemWw {
  public:
    HlSatherString(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlLatexTag : public HlItem {
  public:
    HlLatexTag(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlLatexChar : public HlItem {
  public:
    HlLatexChar(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
};

class HlLatexParam : public HlItem {
  public:
    HlLatexParam(int attribute, int context);
    virtual const QChar *checkHgl(const QChar *,bool);
    virtual bool endEnable(QChar c) {return !isInWord(c);}
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
//    void setData(const ItemStyle &);
    QColor col;
    QColor selCol;
    int bold;   //boolean value
    int italic; //boolean value
};

typedef QList<ItemStyle> ItemStyleList;

//Item Font: family, size, charset
class ItemFont {
  public:
    ItemFont();
//    ItemFont(const ItemFont &);
//    ItemFont(const QString & family, int size, const char *charset);
//    void setData(const ItemFont &);
    QString family;
    int size;
    QString charset;
};

//Item Properties: name, Item Style, Item Font
class ItemData : public ItemStyle, public ItemFont {
  public:
    ItemData(const QString  name, int defStyleNum);
    ItemData(const QString  name, int defStyleNum,
      const QColor&, const QColor&, bool bold, bool italic);

    const QString name;
    int defStyleNum;
    int defStyle; //boolean value
    int defFont;  //boolean value
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


class CHighlight : public GenHighlight {
  public:
    CHighlight(const char * name);
    virtual ~CHighlight();
    virtual QString getCommentStart() { return QString("/*"); };
    virtual QString getCommentEnd() { return QString("*/"); };
  protected:
    virtual void createItemData(ItemDataList &);
    virtual void makeContextList();
    virtual void setKeywords(HlKeyword *keyword, HlKeyword *dataType);
};


class CppHighlight : public CHighlight {
  public:
    CppHighlight(const char * name);
    virtual ~CppHighlight();
    virtual QString getCommentStart() { return QString("//"); };
    virtual QString getCommentEnd() { return QString(""); };
  protected:
    virtual void setKeywords(HlKeyword *keyword, HlKeyword *dataType);
};

class PascalHighlight : public CHighlight {
  public:
    PascalHighlight(const char *name);
    virtual ~PascalHighlight();
    virtual QString getCommentStart() { return QString("//"); };
    virtual QString getCommentEnd() { return QString(""); };
  protected:
    virtual void createItemData(ItemDataList &);
    virtual void makeContextList();
    virtual void setKeywords(HlKeyword *keyword, HlKeyword *dataType);
};

class PovrayHighlight : public CHighlight {   
 public:     
  PovrayHighlight(const char *name);
  virtual ~PovrayHighlight();
 protected:
  virtual void createItemData(ItemDataList &); 
  virtual void makeContextList();
  virtual void setKeywords(HlKeyword *keyword, HlKeyword *dataType);
};

class ObjcHighlight : public CHighlight {
  public:
    ObjcHighlight(const char * name);
    virtual ~ObjcHighlight();
    virtual QString getCommentStart() { return QString("//"); };
    virtual QString getCommentEnd() { return QString(""); };
  protected:
    virtual void makeContextList();
    virtual void setKeywords(HlKeyword *keyword, HlKeyword *dataType);
};

class IdlHighlight : public CHighlight {
  public:
    IdlHighlight(const char * name);
    virtual ~IdlHighlight();
    virtual QString getCommentStart() { return QString("//"); };
    virtual QString getCommentEnd() { return QString(""); };
  protected:
    virtual void setKeywords(HlKeyword *keyword, HlKeyword *dataType);
};

class JavaHighlight : public CHighlight {
  public:
    JavaHighlight(const char * name);
    virtual ~JavaHighlight();
    virtual QString getCommentStart() { return QString("//"); };
    virtual QString getCommentEnd() { return QString(""); };
  protected:
    virtual void setKeywords(HlKeyword *keyword, HlKeyword *dataType);
};

class HtmlHighlight : public GenHighlight {
  public:
    HtmlHighlight(const char * name);
    virtual ~HtmlHighlight();
    virtual QString getCommentStart() { return QString("<!--"); };
    virtual QString getCommentEnd() { return QString("-->"); };
  protected:
    virtual void createItemData(ItemDataList &);
    virtual void makeContextList();
};

class BashHighlight : public GenHighlight {
  public:
    BashHighlight(const char * name);
    virtual ~BashHighlight();
    virtual QString getCommentStart() { return QString("#"); };
  protected:
    virtual void createItemData(ItemDataList &);
    virtual void makeContextList();
};

class ModulaHighlight : public GenHighlight {
  public:
    ModulaHighlight(const char * name);
    virtual ~ModulaHighlight();
    virtual QString getCommentStart() { return QString("(*"); };
    virtual QString getCommentEnd() { return QString("*)"); };
  protected:
    virtual void createItemData(ItemDataList &);
    virtual void makeContextList();
};

class AdaHighlight : public GenHighlight {
  public:
    AdaHighlight(const char * name);
    virtual ~AdaHighlight();
    virtual QString getCommentStart() { return QString("--"); };
  protected:
    virtual void createItemData(ItemDataList &);
    virtual void makeContextList();
};

class PythonHighlight : public GenHighlight {
  public:
    PythonHighlight(const char * name);
    virtual ~PythonHighlight();
    virtual QString getCommentStart() { return QString("#"); };
  protected:
    virtual void createItemData(ItemDataList &);
    virtual void makeContextList();
};

class PerlHighlight : public Highlight {
  public:
    PerlHighlight(const char * name);

    virtual int doHighlight(int ctxNum, TextLine*);
    virtual QString getCommentStart() { return QString("#"); };
  protected:
    virtual void createItemData(ItemDataList &);
    virtual void init();
    virtual void done();
    HlKeyword *keyword;
};

class SatherHighlight : public GenHighlight {
  public:
    SatherHighlight(const char * name);
    virtual ~SatherHighlight();
    virtual QString getCommentStart() { return QString("--"); };
  protected:
    virtual void createItemData(ItemDataList &);
    virtual void makeContextList();
};

class LatexHighlight : public GenHighlight {
  public:
    LatexHighlight(const char * name);
    virtual ~LatexHighlight();
    virtual QString getCommentStart() { return QString("%"); };
  protected:
    virtual void createItemData(ItemDataList &);
    virtual void makeContextList();
};

class KBasicHighlight : public GenHighlight {
  public:
    KBasicHighlight(const char *name);
    virtual ~KBasicHighlight();
    virtual QString getCommentStart() {return QString("\"");};
    virtual QString getCommentEnd()  {return QString("");};

  protected:
    virtual void createItemData(ItemDataList &);
    virtual void makeContextList();
    virtual void setKeywords(HlKeyword *keyword,HlKeyword *dataType);
};

class AutoHighlight : public GenHighlight
{
  public:
    AutoHighlight(syntaxModeListItem *def);
    virtual ~AutoHighlight();
    virtual QString getCommentStart() {return QString("\"");};
    virtual QString getCommentEnd()  {return QString("");};

  protected:
    QString iName;
    QString casesensitive;
    virtual void makeContextList ();
    virtual void setKeywords (HlKeyword *keyword,HlKeyword *dataType);
    virtual void createItemData (ItemDataList &list);
    HlItem *createHlItem(struct syntaxContextData *data, int *res);
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
    void getDefaults(ItemStyleList &, ItemFont &);
    void setDefaults(ItemStyleList &, ItemFont &);

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
