
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

#include <string.h>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <qfile.h>
#include <qlabel.h>

#include <qlayout.h>
#include <qgrid.h>
#include <qhbox.h>
#include <qvgroupbox.h>

#include <kconfig.h>
#include <kglobal.h>
#include <kinstance.h>
#include <kmimemagic.h>
#include <klocale.h>
#include <kregexp.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kstddirs.h>

#include <qpushbutton.h>
#include <qhbox.h>
#include <qvbox.h>
#include <klistview.h>
#include <qwidgetstack.h>

#include "katetextline.h"
#include "katedocument.h"
#include "katehighlight.h"
#include "katesyntaxdocument.h"
#include "../factory/katefactory.h"
#include "katedialogs.h"
HlManager *HlManager::s_pSelf = 0;



enum Item_styles { dsNormal,dsKeyword,dsDataType,dsDecVal,dsBaseN,dsFloat,
                   dsChar,dsString,dsComment,dsOthers};

int getDefStyleNum(QString name)
  {
	if (name=="dsNormal") return dsNormal;
        if (name=="dsKeyword") return dsKeyword;
        if (name=="dsDataType") return dsDataType;
        if (name=="dsDecVal") return dsDecVal;
        if (name=="dsBaseN") return dsBaseN;
        if (name=="dsFloat") return dsFloat;
        if (name=="dsChar") return dsChar;
        if (name=="dsString") return dsString;
        if (name=="dsComment") return dsComment;
        if (name=="dsOthers")  return dsOthers;
	return dsNormal;
  }

bool isInWord(QChar ch) {
  return ch.isLetter() || ch.isDigit() || ch == '_';
/*  static unsigned char data[] = {0,0,0,0,0,0,255,3,254,255,255,135,254,255,255,7};
  if (c & 128) return false;
  return !(data[c >> 3] & (1 << (c & 7)));*/
}

bool ucmp(const QChar *u, const char *s, int len) {
  while (len > 0) {
    if (*u != *s) return false;
    u++;
    s++;
    len--;
  }
  return true;
}

bool ustrchr(const char *s, QChar c) {
  while (*s != '\0') {
    if (*s == c) return true;
    s++;
  }
  return false;
}





HlItem::HlItem(int attribute, int context)
  : attr(attribute), ctx(context)  {subItems=0;
}

HlItem::~HlItem()
{
  //kdDebug(13010)<<"In hlItem::~HlItem()"<<endl;
  if (subItems!=0) {subItems->setAutoDelete(true); subItems->clear(); delete subItems;}
}

HlItemWw::HlItemWw(int attribute, int context)
  : HlItem(attribute,context) {
}


HlCharDetect::HlCharDetect(int attribute, int context, QChar c)
  : HlItem(attribute,context), sChar(c) {
}

const QChar *HlCharDetect::checkHgl(const QChar *str,bool) {
  if (*str == sChar) return str + 1;
  return 0L;
}

Hl2CharDetect::Hl2CharDetect(int attribute, int context, QChar ch1, QChar ch2)
  : HlItem(attribute,context) {
  sChar1 = ch1;
  sChar2 = ch2;
}

const QChar *Hl2CharDetect::checkHgl(const QChar *str,bool) {
  if (str[0] == sChar1 && str[1] == sChar2) return str + 2;
  return 0L;
}

HlStringDetect::HlStringDetect(int attribute, int context, const QString &s, bool inSensitive)
  : HlItem(attribute, context), str(inSensitive ? s.upper():s), _inSensitive(inSensitive) {
}

HlStringDetect::~HlStringDetect() {
}

const QChar *HlStringDetect::checkHgl(const QChar *s,bool) {
  if (!_inSensitive) {if (memcmp(s, str.unicode(), str.length()*sizeof(QChar)) == 0) return s + str.length();}
     else
       {
	 QString tmp=QString(s,str.length()).upper();
	 if (tmp==str) return s+str.length();
       }
  return 0L;
}


HlRangeDetect::HlRangeDetect(int attribute, int context, QChar ch1, QChar ch2)
  : HlItem(attribute,context) {
  sChar1 = ch1;
  sChar2 = ch2;
}

const QChar *HlRangeDetect::checkHgl(const QChar *s,bool) {
  if (*s == sChar1) {
    do {
      s++;
      if (*s == '\0') return 0L;
    } while (*s != sChar2);
    return s + 1;
  }
  return 0L;
}

/*
KeywordData::KeywordData(const char *str) {
  len = strlen(str);
  s = new char[len];
  memcpy(s,str,len);
}

KeywordData::~KeywordData() {
  delete s;
}
*/
HlKeyword::HlKeyword(int attribute, int context,bool casesensitive)
  : HlItemWw(attribute,context) {
//  words.setAutoDelete(true);
// after reading over the docs for Dict
// 23 is probably too small when we can have > 100 items
        if (casesensitive)
          {
            kdDebug()<<"Case Sensitive KeyWord";
            doCheckHgl=&HlKeyword::sensitiveCheckHgl;
          } else
          {
            kdDebug()<<"Case insensitive Keyword";
            doCheckHgl=&inSensitiveCheckHgl;
          }
        _caseSensitive=casesensitive;
        QDict<char> dict(113,casesensitive);
	Dict=dict;
}

HlKeyword::~HlKeyword() {
}

// If we use a dictionary for lookup we don't really need
// an item as such we are using the key to lookup
void HlKeyword::addWord(const QString &word)
{
  words.append(word);
  Dict.insert(word,"dummy");
}
void HlKeyword::addList(const QStringList& list)
{
 if (_caseSensitive)
 {
 words+=list;
 for(uint i=0;i<list.count();i++) Dict.insert(list[i],"dummy");
 }
 else
 {
 words+=list;
 for(uint i=0;i<list.count();i++)
	Dict.insert(list[i].lower(),"dummy");
 }
}

void HlKeyword::addList(const char **list) {
  if (_caseSensitive)
  while (*list) {
    words.append(*list);
    Dict.insert(*list,"dummy");
    list++;
  }
  else
  while (*list) {
    words.append(*list);
    Dict.insert(QString(*list).lower(),"dummy");
    list++;
  }
}

const QChar *HlKeyword::checkHgl(const QChar *s,bool b)
{
  return doCheckHgl(s,b,this);
  //sensitiveCheckHgl(s,b);
}

const QChar *HlKeyword::inSensitiveCheckHgl(const QChar *s,bool,HlKeyword *kw) {
  const QChar *s2=s;
  if(*s2=='\0') return 0L;
  while( !ustrchr("!%&()*+,-./:;<=>?[]^{|}~ ", *s2) && *s2 != '\0') s2++;
// oops didn't increment s2 why do anything else ?
  if(s2 == s) return 0L;
  QString lookup=QString(s,s2-s)+QString::null;
  return kw->Dict[lookup.lower()] ? s2 : 0L;

}

const QChar *HlKeyword::sensitiveCheckHgl(const QChar *s,bool,HlKeyword *kw) {
// this seems to speed up the lookup of keywords somewhat
// anyway it has to be better than iterating through the list

  const QChar *s2=s;

  while( !ustrchr("!%&()*+,-./:;<=>?[]^{|}~ \t", *s2) && *s2 != '\0') s2++;
// oops didn't increment s2 why do anything else?
	if(s2 == s) return 0L;
  QString lookup=QString(s,s2-s)+QString::null;
  return kw->Dict[lookup] ? s2 : 0L;
}


HlInt::HlInt(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlInt::checkHgl(const QChar *str,bool) {
  const QChar *s,*s1;

  s = str;
  while (s->isDigit()) s++;
  if (s > str)
   {
     if (subItems)
       {
	 for (HlItem *it=subItems->first();it;it=subItems->next())
          {
            s1=it->checkHgl(s,false);
	    if (s1) return s1;
          }
       }
     return s;
  }
  return 0L;
}

HlFloat::HlFloat(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlFloat::checkHgl(const QChar *s,bool) {
  bool b, p;
  const QChar *s1;

  b = false;
  while (s->isDigit()){
    s++;
    b = true;
  }
  if (p = (*s == '.')) {
    s++;
    while (s->isDigit()) {
      s++;
      b = true;
    }
  }
  if (!b) return 0L;
  if ((*s&0xdf) == 'E') s++;
    else
      if (!p) return 0L;
	else
	{
          if (subItems)
            {
	      for (HlItem *it=subItems->first();it;it=subItems->next())
                {
                  s1=it->checkHgl(s,false);
	          if (s1) return s1;
                }
            }
          return s;
        }
  if ((*s == '-')||(*s =='+'))  s++;
  b = false;
  while (s->isDigit()) {
    s++;
    b = true;
  }
  if (b)
    {
      if (subItems)
        {
          for (HlItem *it=subItems->first();it;it=subItems->next())
            {
              s1=it->checkHgl(s,false);
              if (s1) return s1;
            }
        }
      return s;
    }
   else return 0L;
}


HlCInt::HlCInt(int attribute, int context)
  : HlInt(attribute,context) {
}

const QChar *HlCInt::checkHgl(const QChar *s,bool lineStart) {

//  if (*s == '0') s++; else s = HlInt::checkHgl(s);
  s = HlInt::checkHgl(s,lineStart);
  if (s != 0L) {
    int l = 0;
    int u = 0;
    const QChar *str;

    do {
      str = s;
      if ((*s&0xdf) == 'L' ) {
        l++;
        if (l > 2) return 0L;
        s++;
      }
      if ((*s&0xdf) == 'U' ){
        u++;
        if (u > 1) return 0L;
        s++;
      }
    } while (s != str);
  }
  return s;
}

HlCOct::HlCOct(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlCOct::checkHgl(const QChar *str,bool) {
  const QChar *s;

  if (*str == '0') {
    str++;
    s = str;
    while (*s >= '0' && *s <= '7') s++;
    if (s > str) {
      if ((*s&0xdf) == 'L' || (*s&0xdf) == 'U' ) s++;
      return s;
    }
  }
  return 0L;
}

HlCHex::HlCHex(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlCHex::checkHgl(const QChar *str,bool) {
  const QChar *s=str;
#if 0
  int i;
  for (i=0;(*s)!='\0';s++,i++);
  QString line(str,i);
  QRegExp3 rx("0[xX][a-fA-F\\d]+[UuLl]?"); // this matches but is also matching parenthesis
  int pos=rx.search(line,0);
  if(pos > -1) return str+rx.matchedLength();
  else
	return 0L;

#else
  if (str[0] == '0' && ((str[1]&0xdf) == 'X' )) {
    str += 2;
    s = str;
    while (s->isDigit() || ((*s&0xdf) >= 'A' && (*s&0xdf) <= 'F') /*|| (*s >= 'a' && *s <= 'f')*/) s++;
    if (s > str) {
      if ((*s&0xdf) == 'L' || (*s&0xdf) == 'U' ) s++;
      return s;
    }
  }
  return 0L;
#endif
}

HlCFloat::HlCFloat(int attribute, int context)
  : HlFloat(attribute,context) {
}

const QChar *HlCFloat::checkHgl(const QChar *s,bool lineStart) {

  s = HlFloat::checkHgl(s,lineStart);
  if (s && ((*s&0xdf) == 'F' )) s++;
  return s;
}

HlAnyChar::HlAnyChar(int attribute, int context, char* charList)
  : HlItem(attribute, context) {
  _charList=charList;
}

const QChar *HlAnyChar::checkHgl(const QChar *s,bool) {
  //kdDebug(13010)<<"in AnyChar::checkHgl: _charList: "<<_charList<<endl;
  if (ustrchr(_charList, *s)) return s +1;
  return 0L;
}

HlRegExpr::HlRegExpr(int attribute, int context,QString regexp)
  : HlItem(attribute, context) {

    handlesLinestart=regexp.startsWith("^");
    if(!handlesLinestart) regexp.prepend("^");
    Expr=new QRegExp3(regexp);
}

const QChar *HlRegExpr::checkHgl(const QChar *s,bool lineStart)
{
  if ((!lineStart) && handlesLinestart) return 0;
  //kdDebug(13010)<<"Trying to match:"<<Expr->pattern()<<endl;
  const QChar *chtmp=s;
  int i;
  for (i=0;(*chtmp)!='\0';chtmp++,i++);
  QString line(s,i);
  int pos = Expr->search( line, 0 );
  if (pos==-1) return 0L;
    else return (s+Expr->matchedLength());

/*  int len;
  if (Expr->match(line,0,&len)!=-1)
   {
     return s+len;
   }
  return 0L;*/
};


HlLineContinue::HlLineContinue(int attribute, int context)
  : HlItem(attribute,context) {
}

const QChar *HlLineContinue::checkHgl(const QChar *s,bool) {
  if (*s == '\\') return s + 1;
  return 0L;
}


HlCStringChar::HlCStringChar(int attribute, int context)
  : HlItem(attribute,context) {
}

//checks for hex and oct (for example \x1b or \033)
const QChar *checkCharHexOct(const QChar *str) {
  const QChar *s;
        s=str;
        int n;
  if (*s == 'x') {
    n = 0;
    do {
      s++;
      n *= 16;
      if (s->isDigit()) n += *s - '0';
      else if ((*s&0xdf) >= 'A' && (*s&0xdf) <= 'F') n += (*s&0xdf) - 'A' + 10;
//      else if (*s >= 'a' && *s <= 'f') n += *s - 'a' + 10;
      else break;
      if (n >= 256) return 0L;
    } while (true);
    if (s - str == 1) return 0L;
  } else {
    if (!(*s >= '0' && *s <= '7')) return 0L;
    n = *s - '0';
    do {
      s++;
      n *= 8;
      if (*s >= '0' && *s <= '7') n += *s - '0'; else break;
      if (n >= 256) return s;
    } while (s - str < 3);
  }
  return s;
}
// checks for C escaped chars \n and escaped hex/octal chars
const QChar *checkEscapedChar(const QChar *s) {
  int i;
  if (s[0] == '\\' && s[1] != '\0' ) {
        s++;
        switch(*s){
                case  'a': // checks for control chars
                case  'b': // we want to fall through
                case  'e':
                case  'f':

                case  'n':
                case  'r':
                case  't':
                case  'v':
                case '\'':
                case '\"':
                case '?' : // added ? ANSI C classifies this as an escaped char
                case '\\': s++;
                           break;
                case 'x': // if it's like \xff
                        s++; // eat the x
                        // these for loops can probably be
                        // replaced with something else but
                        // for right now they work
                        // check for hexdigits
                        for(i=0;i<2 &&(*s >= '0' && *s <= '9' || (*s&0xdf) >= 'A' && (*s&0xdf) <= 'F');i++,s++);
                        if(i==0) return 0L; // takes care of case '\x'
                        break;

                case '0': case '1': case '2': case '3' :
                case '4': case '5': case '6': case '7' :
                        for(i=0;i < 3 &&(*s >='0'&& *s<='7');i++,s++);
                        break;
                        default: return 0L;
        }
  return s;
  }
  return 0L;
}

const QChar *HlCStringChar::checkHgl(const QChar *str,bool) {
  return checkEscapedChar(str);
}


HlCChar::HlCChar(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlCChar::checkHgl(const QChar *str,bool) {
  const QChar *s;

  if (str[0] == '\'' && str[1] != '\0' && str[1] != '\'') {
    s = checkEscapedChar(&str[1]); //try to match escaped char
    if (!s) s = &str[2];           //match single non-escaped char
    if (*s == '\'') return s + 1;
  }
  return 0L;
}


//--------
ItemStyle::ItemStyle() : selCol(Qt::white), bold(false), italic(false) {
}

ItemStyle::ItemStyle(const QColor &col, const QColor &selCol,
  bool bold, bool italic)
  : col(col), selCol(selCol), bold(bold), italic(italic) {
}

ItemData::ItemData(const QString  name, int defStyleNum)
  : name(name), defStyleNum(defStyleNum), defStyle(true) {
}

ItemData::ItemData(const QString name, int defStyleNum,
  const QColor &col, const QColor &selCol, bool bold, bool italic)
  : ItemStyle(col,selCol,bold,italic), name(name), defStyleNum(defStyleNum),
  defStyle(false) {
}

HlData::HlData(const QString &wildcards, const QString &mimetypes, const QString &identifier)
  : wildcards(wildcards), mimetypes(mimetypes), identifier(identifier) {

  itemDataList.setAutoDelete(true);
}

Highlight::Highlight(const char * name) : iName(name), refCount(0)
{


}

Highlight::~Highlight() {

}

KConfig *Highlight::getKConfig() {
  KConfig *config;

  config = KateFactory::instance()->config();
  config->setGroup(QString::fromUtf8(iName) + QString::fromUtf8(" Highlight"));
  return config;
}

QString Highlight::getWildcards() {
  KConfig *config;

  config = getKConfig();

  //if wildcards not yet in config, then use iWildCards as default
  return config->readEntry("Wildcards", iWildcards);
}


QString Highlight::getMimetypes() {
  KConfig *config;

  config = getKConfig();

  return config->readEntry("Mimetypes", iMimetypes);
}


HlData *Highlight::getData() {
  KConfig *config;
  HlData *hlData;

  config = getKConfig();

//  iWildcards = config->readEntry("Wildcards");
//  iMimetypes = config->readEntry("Mimetypes");
//  hlData = new HlData(iWildcards,iMimetypes);
  hlData = new HlData(
    config->readEntry("Wildcards", iWildcards),
    config->readEntry("Mimetypes", iMimetypes),
    config->readEntry("Identifier", identifier));
  getItemDataList(hlData->itemDataList, config);
  return hlData;
}

void Highlight::setData(HlData *hlData) {
  KConfig *config;

  config = getKConfig();

//  iWildcards = hlData->wildcards;
//  iMimetypes = hlData->mimetypes;

  config->writeEntry("Wildcards",hlData->wildcards);
  config->writeEntry("Mimetypes",hlData->mimetypes);

  setItemDataList(hlData->itemDataList,config);
}

void Highlight::getItemDataList(ItemDataList &list) {
  KConfig *config;

  config = getKConfig();
  getItemDataList(list, config);
}

void Highlight::getItemDataList(ItemDataList &list, KConfig *config) {
  ItemData *p;
  QString s;
  QRgb col, selCol;

  list.clear();
  list.setAutoDelete(true);
  createItemData(list);

  for (p = list.first(); p != 0L; p = list.next()) {
    s = config->readEntry(p->name);
    if (!s.isEmpty()) {
      sscanf(s.latin1(),"%d,%X,%X,%d,%d", &p->defStyle,&col,&selCol,&p->bold,&p->italic);
      p->col.setRgb(col);
      p->selCol.setRgb(selCol);
    }
  }
}

void Highlight::setItemDataList(ItemDataList &list, KConfig *config) {
  ItemData *p;
  QString s;

  for (p = list.first(); p != 0L; p = list.next()) {
    s.sprintf("%d,%X,%X,%d,%d",
      p->defStyle,p->col.rgb(),p->selCol.rgb(),p->bold,p->italic);
    config->writeEntry(p->name,s);
  }
}

void Highlight::use() {
  if (refCount == 0) init();
  refCount++;
}

void Highlight::release() {
  refCount--;
  if (refCount == 0) done();
}

/*
bool Highlight::isInWord(char ch) {
  static char data[] = {0,0,0,0,0,0,255,3,254,255,255,135,254,255,255,7};
  if (ch & 128) return true;
  return data[ch >> 3] & (1 << (ch & 7));
}
*/
int Highlight::doHighlight(int, TextLine *textLine) {

  textLine->setAttribs(0,0,textLine->length());
  textLine->setAttr(0);
  return 0;
}

void Highlight::createItemData(ItemDataList &list) {

  list.append(new ItemData(I18N_NOOP("Normal Text"), dsNormal));
}


void Highlight::init() {
}

void Highlight::done() {
}


HlContext::HlContext(int attribute, int lineEndContext)
  : attr(attribute), ctx(lineEndContext) {
  items.setAutoDelete(true);
}


GenHighlight::GenHighlight(const char * name) : Highlight(name) {
}


int GenHighlight::doHighlight(int ctxNum, TextLine *textLine) {
  HlContext *context;
  const QChar *str, *s1, *s2;
  QChar lastChar;
  HlItem *item;

  context = contextList[ctxNum];
  str = textLine->getString();
  lastChar = '\0';
// this causes the while loop to skip any spaces at beginning of line
// while still allowing the highlighting to continue
// On limited tests I got a 5-10% reduction in number of times in while loop
// Anything helps :)
 s1=textLine->firstNonSpace();

  while (*s1 != '\0') {
    for (item = context->items.first(); item != 0L; item = context->items.next()) {
      if (item->startEnable(lastChar)) {
        s2 = item->checkHgl(s1,s1==str);
        if (s2 > s1) {
          if (item->endEnable(*s2)) { //jowenn: Here I've to change a lot
            textLine->setAttribs(item->attr,s1 - str,s2 - str);
            ctxNum = item->ctx;
            context = contextList[ctxNum];
            s1 = s2 - 1;
            goto found;
          }
        }
      }
    }
    // nothing found: set attribute of one char
    textLine->setAttribs(context->attr,s1 - str,s1 - str + 1);

    found:
    lastChar = *s1;
    s1++;
  }
  //set "end of line"-properties
  textLine->setAttr(context->attr);
  //return new context
  return context->ctx;
}

void GenHighlight::init() {
  int z;

  for (z = 0; z < nContexts; z++) contextList[z] = 0L;
  makeContextList();
}

void GenHighlight::done() {
  int z;

  for (z = 0; z < nContexts; z++) delete contextList[z];
}


Hl2CharDetect::Hl2CharDetect(int attribute, int context, const QChar *s)
  : HlItem(attribute,context) {
  sChar1 = s[0];
  sChar2 = s[1];
}

#if 0
HlCaseInsensitiveKeyword::HlCaseInsensitiveKeyword(int attribute, int context)
  : HlKeyword(attribute,context) {
// make dictionary case insensitive
  QDict<char> dict(113,false);
  Dict=dict;
}

HlCaseInsensitiveKeyword::~HlCaseInsensitiveKeyword() {
}

void HlCaseInsensitiveKeyword::addList(const QStringList& lst)
{
 words+=lst;
 for(uint i=0;i<lst.count();i++)
	Dict.insert(lst[i].lower(),"dummy");
}
void HlCaseInsensitiveKeyword::addList(const char **list)
{
  while (*list) {
    words.append(*list);
    Dict.insert(QString(*list).lower(),"dummy");
    list++;
  }
}
const QChar *HlCaseInsensitiveKeyword::checkHgl(const QChar *s,bool)
{
  const QChar *s2=s;
  if(*s2=='\0') return 0L;
  while( !ustrchr("!%&()*+,-./:;<=>?[]^{|}~ ", *s2) && *s2 != '\0') s2++;
// oops didn't increment s2 why do anything else ?
  if(s2 == s) return 0L;
  QString lookup=QString(s,s2-s)+QString::null;
  return Dict[lookup.lower()] ? s2 : 0L;
}

/*
   Not really tested but I assume it will work
*/
const char *HlCaseInsensitiveKeyword::checkHgl(const char *s,bool) {

// if s is in dictionary then return s+strlen(s)
   return Dict[s] ? s+strlen(s) : 0L;
}

#endif

AutoHighlight::AutoHighlight(syntaxModeListItem *def):GenHighlight(def->name.latin1())
{
//  syntaxContextData *data;
  iName = def->name;
  iWildcards = def->extension;
  iMimetypes = def->mimetype;
  identifier = def->identifier;
  casesensitive = def->casesensitive;
}

AutoHighlight::~AutoHighlight()
{
}

void AutoHighlight::setKeywords(HlKeyword *keyword, HlKeyword *dataType)
{
//  if (casesensitive=="1")
  {
   if (keyword) keyword->addList(HlManager::self()->syntax->finddata("highlighting","keywords"));
   if (dataType) dataType->addList(HlManager::self()->syntax->finddata("highlighting","types"));
  }
}

void AutoHighlight::createItemData(ItemDataList &list)
{
  struct syntaxContextData *data;
  //kdDebug(13010)<<"In AutoHighlight::createItemData***********************"<<endl;
  HlManager::self()->syntax->setIdentifier(identifier);
  data=HlManager::self()->syntax->getGroupInfo("highlighting","itemData");
  while (HlManager::self()->syntax->nextGroup(data))
    {
	//kdDebug(13010)<< HlManager::self()->syntax->groupData(data,QString("name"))<<endl;
	list.append(new ItemData(
          HlManager::self()->syntax->groupData(data,QString("name")),
          getDefStyleNum(HlManager::self()->syntax->groupData(data,QString("defStyleNum")))));

    }
  if (data) HlManager::self()->syntax->freeGroupInfo(data);
  //kdDebug(13010)<<"****************ITEMDATALIST - BEGIN*********************"<<endl;
/*    for (ItemData *it=list.first();it!=0;it=list.next())
      {
        kdDebug(13010)<<it->name<<endl;
      }*/
//  kdDebug(13010)<<"****************ITEMDATALIST - END*********************"<<endl;
}

HlItem *AutoHighlight::createHlItem(struct syntaxContextData *data, int *res)
{

                QString dataname=HlManager::self()->syntax->groupItemData(data,QString("name"));
                int attr=((HlManager::self()->syntax->groupItemData(data,QString("attribute"))).toInt());
                int context=((HlManager::self()->syntax->groupItemData(data,QString("context"))).toInt());
		char chr;
                if (! HlManager::self()->syntax->groupItemData(data,QString("char")).isEmpty())
		  chr= (HlManager::self()->syntax->groupItemData(data,QString("char")).latin1())[0];
		else
                  chr=0;
		QString stringdata=HlManager::self()->syntax->groupItemData(data,QString("String"));
                char chr1;
                if (! HlManager::self()->syntax->groupItemData(data,QString("char1")).isEmpty())
		  chr1= (HlManager::self()->syntax->groupItemData(data,QString("char1")).latin1())[0];
		else
                  chr1=0;
		bool insensitive=(HlManager::self()->syntax->groupItemData(data,QString("insensitive"))==QString("TRUE"));
		*res=0;
                if (dataname=="keyword") 
		{
	           HlKeyword *keyword=new HlKeyword(attr,context,casesensitive=="1");
		   keyword->addList(HlManager::self()->syntax->finddata("highlighting",stringdata));  
		   return keyword;
		} else
//                if (dataname=="dataType") {*res=2; return(new HlKeyword(attr,context,casesensitive=="1"));} else
                if (dataname=="Float") return (new HlFloat(attr,context)); else
                if (dataname=="Int") return(new HlInt(attr,context)); else
                if (dataname=="CharDetect") return(new HlCharDetect(attr,context,chr)); else
                if (dataname=="2CharDetect") return(new Hl2CharDetect(attr,context,chr,chr1)); else
                if (dataname=="RangeDetect") return(new HlRangeDetect(attr,context, chr, chr1)); else
		if (dataname=="LineContinue") return(new HlLineContinue(attr,context)); else
                if (dataname=="StringDetect") return(new HlStringDetect(attr,context,stringdata,insensitive)); else
                if (dataname=="AnyChar") return(new HlAnyChar(attr,context,(char*)stringdata.latin1())); else
                if (dataname=="RegExpr") return(new HlRegExpr(attr,context,stringdata)); else
// apparently these were left out
	   if(dataname=="HlCChar") return ( new HlCChar(attr,context));else
      if(dataname=="HlCHex") return (new HlCHex(attr,context));else
	  if(dataname=="HlCOct") return (new HlCOct(attr,context)); else
	  if(dataname=="HlCStringChar") return (new HlCStringChar(attr,context)); else

                  {
//                    kdDebug(13010)<< k_lineinfo "****************** "<<endl<<"Unknown entry for Context:"<<dataname<<endl;
                    return 0;
                  }


}

void AutoHighlight::makeContextList()
{
  HlKeyword *keyword=0, *dataType=0;
  struct syntaxContextData *data, *datasub;
  HlItem *c;

  kdDebug(13010)<< "AutoHighlight makeContextList()"<<endl;
  HlManager::self()->syntax->setIdentifier(identifier);

  cslStart = "";

  data=HlManager::self()->syntax->getGroupInfo("general","comment");
  if (data)
    {
      kdDebug()<<"COMMENT DATA FOUND"<<endl;
    while  (HlManager::self()->syntax->nextGroup(data))
      {
        kdDebug()<<HlManager::self()->syntax->groupData(data,"name")<<endl;
        if (HlManager::self()->syntax->groupData(data,"name")=="singleLine")
		cslStart=HlManager::self()->syntax->groupData(data,"start");
	if (HlManager::self()->syntax->groupData(data,"name")=="multiLine")
           {
		cmlStart=HlManager::self()->syntax->groupData(data,"start");
		cmlEnd=HlManager::self()->syntax->groupData(data,"end");
           }
      }
    }

  if (data) HlManager::self()->syntax->freeGroupInfo(data);
  data=0;

  data=HlManager::self()->syntax->getGroupInfo("highlighting","context");
  int i=0;
  if (data)
    {
      while (HlManager::self()->syntax->nextGroup(data))
        {
//	kdDebug(13010)<< "In make Contextlist: Group"<<endl;
          contextList[i]=new HlContext(
            (HlManager::self()->syntax->groupData(data,QString("attribute"))).toInt(),
            (HlManager::self()->syntax->groupData(data,QString("lineEndContext"))).toInt());


            while (HlManager::self()->syntax->nextItem(data))
              {
//		kdDebug(13010)<< "In make Contextlist: Item:"<<endl;

		int res;
		c=createHlItem(data,&res);
		if (c)
			{
				contextList[i]->items.append(c);
				if (res==1) keyword=(HlKeyword*)c; else if (res==2) dataType=(HlKeyword*)c;

				datasub=HlManager::self()->syntax->getSubItems(data);
				bool tmpbool;
				if (tmpbool=HlManager::self()->syntax->nextItem(datasub))
					{
                                          c->subItems=new QList<HlItem>;
					  for (;tmpbool;tmpbool=HlManager::self()->syntax->nextItem(datasub))
                                            c->subItems->append(createHlItem(datasub,&res));
                                        }
				HlManager::self()->syntax->freeGroupInfo(datasub);
			}
//		kdDebug(13010)<<"Last line in loop"<<endl;
              }
          i++;
        }
      }
//  kdDebug(13010)<<"After creation loop in AutoHighlight::makeContextList"<<endl;
  HlManager::self()->syntax->freeGroupInfo(data);
  setKeywords(keyword, dataType);
//  kdDebug(13010)<<"After setKeyWords AutoHighlight::makeContextList"<<endl;

}

//--------



HlManager::HlManager() : QObject(0L)
{
  syntax = new SyntaxDocument();
  SyntaxModeList modeList = syntax->modeList();

  hlList.setAutoDelete(true);
  hlList.append(new Highlight(I18N_NOOP("Normal")));

   /* new stuff*/
 uint i=0;
  while (i < modeList.count())
  {
    hlList.append(new AutoHighlight(modeList.at(i)));
    i++;
  }

/*
  kdDebug(13010)<<"Creating HlList"<<endl;
  hlList.append(new CHighlight(     "C"        ));
  hlList.append(new CppHighlight(   "C++"      ));
  hlList.append(new ObjcHighlight(  "Objective-C"));
  hlList.append(new JavaHighlight(  "Java"     ));
  hlList.append(new HtmlHighlight(  "HTML"     ));
  hlList.append(new BashHighlight(  "Bash"     ));
  hlList.append(new ModulaHighlight("Modula 2" ));
  hlList.append(new AdaHighlight(   "Ada"      ));
  hlList.append(new PascalHighlight("Pascal"   ));
  hlList.append(new PovrayHighlight("Povray"   ));
  hlList.append(new PythonHighlight("Python"   ));
  hlList.append(new PerlHighlight(  "Perl"     ));
  hlList.append(new SatherHighlight("Sather"   ));
  hlList.append(new KBasicHighlight("KBasic"));
  hlList.append(new LatexHighlight( "Latex"    ));
  hlList.append(new IdlHighlight("IDL"));
  kdDebug(13010)<<"HlList created"<<endl;
 */
}

HlManager::~HlManager() {
  if(syntax) delete syntax;
}

HlManager *HlManager::self()
{
  if ( !s_pSelf )
    s_pSelf = new HlManager;
  return s_pSelf;
}

Highlight *HlManager::getHl(int n) {
  if (n < 0 || n >= (int) hlList.count()) n = 0;
  return hlList.at(n);
}

int HlManager::defaultHl() {
  KConfig *config;

  config = KateFactory::instance()->config();
  config->setGroup("General Options");
  return nameFind(config->readEntry("Highlight"));
}


int HlManager::nameFind(const QString &name) {
  int z;

  for (z = hlList.count() - 1; z > 0; z--) {
    if (hlList.at(z)->iName == name) break;
  }
  return z;
}

int HlManager::wildcardFind(const QString &fileName) {
  Highlight *highlight;
  int p1, p2;
  QString w;
  for (highlight = hlList.first(); highlight != 0L; highlight = hlList.next()) {
    p1 = 0;
    w = highlight->getWildcards();
    while (p1 < (int) w.length()) {
      p2 = w.find(';',p1);
      if (p2 == -1) p2 = w.length();
      if (p1 < p2) {
        QRegExp regExp(w.mid(p1,p2 - p1),true,true);
        if (regExp.match(fileName) == 0) return hlList.at();
      }
      p1 = p2 + 1;
    }
  }
  return -1;
}

int HlManager::mimeFind(const QByteArray &contents, const QString &fname)
{
/*
  // fill the detection buffer with the contents of the text
  const int HOWMANY = 1024;
  char buffer[HOWMANY];
  int number=0, len;

  for (int index=0; index<doc->lastLine(); index++)
  {
    len = doc->textLength(index);

    if (number+len > HOWMANY)
      break;

    memcpy(&buffer[number], doc->textLine(index)->getText(), len);
    number += len;
  }
*/
  // detect the mime type
  KMimeMagicResult *result;
  result = KMimeMagic::self()->findBufferFileType(contents, fname);

  Highlight *highlight;
  int p1, p2;
  QString w;

  for (highlight = hlList.first(); highlight != 0L; highlight = hlList.next())
  {
    w = highlight->getMimetypes();

    p1 = 0;
    while (p1 < (int) w.length()) {
      p2 = w.find(';',p1);
      if (p2 == -1) p2 = w.length();
      if (p1 < p2) {
        QRegExp regExp(w.mid(p1,p2 - p1),true,true);
        if (regExp.match(result->mimeType()) == 0) return hlList.at();
      }
      p1 = p2 + 1;
    }
  }

  return -1;
}

int HlManager::makeAttribs(Highlight *highlight, Attribute *a, int maxAttribs) {
  ItemStyleList defaultStyleList;
  ItemStyle *defaultStyle;
  ItemDataList itemDataList;
  ItemData *itemData;
  int nAttribs, z;

  defaultStyleList.setAutoDelete(true);
  getDefaults(defaultStyleList);

  itemDataList.setAutoDelete(true);
  highlight->getItemDataList(itemDataList);
  nAttribs = itemDataList.count();
  for (z = 0; z < nAttribs; z++) {
    itemData = itemDataList.at(z);
    if (itemData->defStyle) {
      // default style
      defaultStyle = defaultStyleList.at(itemData->defStyleNum);
      a[z].col = defaultStyle->col;
      a[z].selCol = defaultStyle->selCol;
      a[z].bold = defaultStyle->bold;
      a[z].italic = defaultStyle->italic;
    } else {
      // custom style
      a[z].col = itemData->col;
      a[z].selCol = itemData->selCol;
      a[z].bold = itemData->bold;
      a[z].italic = itemData->italic;
    }
  }

  for (; z < maxAttribs; z++) {
    a[z].col = black;
    a[z].selCol = black;
    a[z].bold = defaultStyle->bold;
    a[z].italic = defaultStyle->italic;
  }
  return nAttribs;
}

int HlManager::defaultStyles() {
  return 10;
}

const char * HlManager::defaultStyleName(int n) {
  static const char *names[] = {
    I18N_NOOP("Normal"),
    I18N_NOOP("Keyword"),
    I18N_NOOP("Data Type"),
    I18N_NOOP("Decimal/Value"),
    I18N_NOOP("Base-N Integer"),
    I18N_NOOP("Floating Point"),
    I18N_NOOP("Character"),
    I18N_NOOP("String"),
    I18N_NOOP("Comment"),
    I18N_NOOP("Others")};

  return names[n];
}

void HlManager::getDefaults(ItemStyleList &list) {
  KConfig *config;
  int z;
  ItemStyle *i;
  QString s;
  QRgb col, selCol;

  list.setAutoDelete(true);
  //ItemStyle(color, selected color, bold, italic)
  list.append(new ItemStyle(black,white,false,false));     //normal
  list.append(new ItemStyle(black,white,true,false));      //keyword
  list.append(new ItemStyle(darkRed,white,false,false));   //datatype
  list.append(new ItemStyle(blue,cyan,false,false));       //decimal/value
  list.append(new ItemStyle(darkCyan,cyan,false,false));   //base n
  list.append(new ItemStyle(darkMagenta,cyan,false,false));//float
  list.append(new ItemStyle(magenta,magenta,false,false)); //char
  list.append(new ItemStyle(red,red,false,false));         //string
  list.append(new ItemStyle(darkGray,gray,false,true));    //comment
  list.append(new ItemStyle(darkGreen,green,false,false)); //others

  config = KateFactory::instance()->config();
  config->setGroup("Default Item Styles");
  for (z = 0; z < defaultStyles(); z++) {
    i = list.at(z);
    s = config->readEntry(defaultStyleName(z));
    if (!s.isEmpty()) {
      sscanf(s.latin1(),"%X,%X,%d,%d",&col,&selCol,&i->bold,&i->italic);
      i->col.setRgb(col);
      i->selCol.setRgb(selCol);
    }
  }
}

void HlManager::setDefaults(ItemStyleList &list) {
  KConfig *config;
  int z;
  ItemStyle *i;
  char s[64];

  config =  KateFactory::instance()->config();
  config->setGroup("Default Item Styles");
  for (z = 0; z < defaultStyles(); z++) {
    i = list.at(z);
    sprintf(s,"%X,%X,%d,%d",i->col.rgb(),i->selCol.rgb(),i->bold, i->italic);
    config->writeEntry(defaultStyleName(z),s);
  }

  emit changed();
}


int HlManager::highlights() {
  return (int) hlList.count();
}

const char * HlManager::hlName(int n) {
  return hlList.at(n)->iName;
}

void HlManager::getHlDataList(HlDataList &list) {
  int z;

  for (z = 0; z < (int) hlList.count(); z++) {
    list.append(hlList.at(z)->getData());
  }
}

void HlManager::setHlDataList(HlDataList &list) {
  int z;

  for (z = 0; z < (int) hlList.count(); z++) {
    hlList.at(z)->setData(list.at(z));
  }
  //notify documents about changes in highlight configuration
  emit changed();
}


#include "katehighlight.moc"
