/*
  $Id$

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
#include <kinstance.h>
#include <kglobal.h>
#include <kfontdialog.h>
#include <kcharsets.h>
#include <kmimemagic.h>
#include <klocale.h>
#include <kregexp.h>
#include <kglobalsettings.h>
#include <kdebug.h>
#include <kstddirs.h>

#include "kwtextline.h"
#include "kwattribute.h"
#include "kwrite_factory.h"
#include "highlight.h"



// general rule for keywords: if one keyword contains another at the beginning,
// the long one has to come first (eg. "const_cast", "const")

// ISO/IEC 9899:1990 (aka ANSI C)
// "interrupt" isn�t an ANSI keyword, but an extension of some C compilers
const char *cKeywords[] = { // 18 words
  "break", "case", "continue", "default", "do", "else", "enum", "extern",
  "for", "goto", "if", /*"interrupt",*/  "return", "sizeof", "struct",
  "switch", "typedef", "union", "while", 0L};

const char *cTypes[] = {    // 14 words
  "auto", "char", "const", "double", "float", "int", "long", "register",
  "short", "signed", "static", "unsigned", "void", "volatile", 0L};

// ISO/IEC 14882:1998 . Sec. 2.11.1 (aka ANSI C++)
// keyword "const" (apart from a type spec.) is also a keyword, so it is named inside this array
// what about typeof?
const char *cppKeywords[] = { // 40 words 58 with QT_SUPPORT
  "asm", "catch", "class", "const_cast", "const", "delete", "dynamic_cast",
  "explicit", "export", "false", "friend", "inline", "namespace", "new",
  "operator", "private", "protected", "public", "reinterpret_cast",
  "static_cast", "template", "this", "throw", "true", "try", "typeid",
  "typename", "using", "virtual",
  // alternative representations  (these words are reserved and shall not be used otherwise)
  //  ISO/IEC 14882:1998 . Sec. 2.11.2
  "and_eq", "and", "bitand", "bitor", "compl", "not_eq", "not", "or_eq", "or",
  "xor_eq", "xor",
#ifdef QT_SUPPORT
   "bad_cast", "bad_typeid","except","finally","type_info", "xalloc",
 // ADDED FOR TESTING
 "Q_EXPORT","Q_OBJECT","K_DCOP","SLOT","SIGNAL", "slots","signals",
 // QT 2.1 macros
 "Q_PROPERTY", "Q_ENUMS","Q_SETS","Q_OVERRIDE","Q_CLASSINFO",
#endif
  0L};



const char *cppTypes[] = {  //3 words
  "bool", "wchar_t", "mutable", 0L};

const char *objcKeywords[] = { // 13 words
  "@class", "@defs", "@encode", "@end", "@implementation", "@interface", "@private", "@protected",
  "@protocol", "@public","@selector", "self", "super", 0L};

const char *objcTypes[] = {  // 14 words
  "auto", "char", "const", "double", "float", "int", "long", "register",
  "short", "signed", "static",
  "unsigned", "void", "volatile",
  0L};

const char *idlKeywords[] = { // 16 words
  "module", "interface", "struct", "case", "enum", "typedef","signal", "slot",
  "attribute", "readonly", "context", "oneway", "union", "in", "out", "inout",
  0L};

const char *idlTypes[] = {  // 15 words
  "long", "short", "unsigned", "double", "octet", "sequence", "char", "wchar",
  "string", "wstring", "any", "fixed", "Object", "void", "boolean", 0L};

const char *javaKeywords[] = { // 46 words
  "abstract", "break", "case", "cast", "catch", "class", "continue",
  "default", "do", "else", "extends", "false", "finally", "for", "future",
  "generic", "goto", "if", "implements", "import", "inner", "instanceof",
  "interface", "native", "new", "null", "operator", "outer", "package",
  "private", "protected", "public", "rest", "return", "super", "switch",
  "synchronized", "this", "throws", "throw", "transient", "true", "try",
  "var", "volatile", "while", 0L};

const char *javaTypes[] = {  // 12 words
  "boolean", "byte", "char", "const", "double", "final", "float", "int",
  "long", "short", "static", "void", 0L};

const char *bashKeywords[] = {  // 20 words
  "break","case","done","do","elif","else","esac","exit","export","fi","for",
  "function","if","in","return","select","then","until","while",".",0L};

const char *modulaKeywords[] = { // 25 words
  "BEGIN","CONST","DEFINITION","DIV","DO","ELSE","ELSIF","END","FOR","FROM",
  "IF","IMPLEMENTATION","IMPORT","MODULE","MOD","PROCEDURE","RECORD","REPEAT",
  "RETURN","THEN","TYPE","VAR","WHILE","WITH","|",0L};

const char *adaKeywords[] = { // 63 words
  "abort","abs","accept","access","all","and","array","at","begin","body",
  "case","constant","declare","delay","delta","digits","do","else","elsif",
  "end","entry","exception","exit","for", "function","generic","goto","if",
  "in","is","limited","loop","mod","new", "not","null","of","or","others",
  "out","package","pragma","private","procedure", "raise","range","rem",
  "record","renames","return","reverse","select","separate","subtype", "task",
  "terminate","then","type","use","when","while","with","xor",0L};

const char *pythonKeywords[] = { // 28 words
  "and","assert","break","class","continue","def","del","elif","else",
  "except","exec"," finally","for","from","global","if","import","in","is",
  "lambda","not","or","pass","print","raise","return","try","while",0L};

const char *perlKeywords[] = {  // 51 words
  "and","&&", "bless","caller","cmp","continue","dbmclose","dbmopen","do",
  "die", "dump", "each", "else", "elsif","eq","eval", "exit", "foreach",
  "for", "ge", "goto", "gt", "if", "import", "last", "le", "local", "lt",
  "my", "next", "ne", "not", "no", "!", "or", "||", "package", "ref",
  "redo", "require","return","sub", "tied", "tie", "unless",
  "until", "untie", "use", "wantarray", "while", "xor", 0L};

const char *satherKeywords[] = { // 60 words
  "and","assert","attr","break!","case","class","const","else","elsif",
  "end","exception","external","false","if","include","initial","is","ITER",
  "loop","new","or","post","pre","private","protect","quit","raise",
  "readonly","result","return","ROUT","SAME","self","shared","then","true",
  "typecase","type","until!","value","void","when","while!","yield",
// new in 1.1 and pSather:
  "abstract","any","bind","fork","guard","immutable","inout","in","lock",
  "once","out","parloop","partial","par","spread","stub", 0L};

// are those Sather keywords, too?
//     "nil","do@", "do"

const char *satherSpecClassNames[] = {  // 17 words
  "$OB","ARRAY","AREF","AVAL","BOOL","CHAR","EXT_OB","FLTDX","FLTD","FLTX",
  "FLTI","FLT","INTI","INT","$REHASH","STR","SYS",0L};

const char *satherSpecFeatureNames[] = {  // 19 words
// real special features
  "create","invariant","main",
// sugar feature names
  "aget","aset","div","is_eq","is_geq","is_gt","is_leq","is_lt","is_neq",
  "minus","mod","negate","not","plus","pow","times", 0L};
#ifdef PASCAL_SUPPORT
const char *pascalKeywords[] = { // 79 words
  // Ancient DOS Turbo Pascal keywords:
  //"absolute", "out",
  // Generic pascal keywords:
  "and", "array", "asm", "begin", "case", "const", "div", "do", "downto", "else",
  "end", "for", "function", "goto", "if", "implementation", "in", "interface",
  "label", "mod", "nil", "not", "of", "on", "operator", "or", "packed",
  "procedure", "program", "record", "repeat", "self", "set", "shl", "shr", "then",
  "to", "type", "unit", "until", "uses", "var", "while", "with", "xor",
  // Borland pascal keywords:
  "break", "continue", "constructor", "destructor", "inherited", "inline", "object",
  "private","protected" , "public",
  // Borland Delphi keywords
  "as", "at", "automated", "class", "dispinterface", "except", "exports",
  "finalization", "finally", "initialization", "is", "library", "on", "property",
  "published", "raise", "resourcestring", "threadvar", "try",
  // FPC keywords (www.freepascal.org)
  "dispose", "exit", "false", "new", "true",
  0L};

const char *pascalTypes[] = { // 31 words
  "Integer", "Cardinal",
  "ShortInt", "SmallInt", "LongInt", "Int64", "Byte", "Word", "LongWord",
  "Char", "AnsiChar", "WideChar",
  "Boolean", "ByteBool", "WordBool", "LongBool",
  "Single", "Double", "Extended", "Comp",  "Currency", "Real", "Real48"
  "String", "ShortString", "AnsiString", "WideString",
  "Pointer", "Variant",
  "File", "Text",
  0L};
#endif

char fontSizes[] = {4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,22,24,26,28,32,48,64,0};

enum Item_styles { dsNormal,dsKeyword,dsDataType,dsDecVal,dsBaseN,dsFloat,
                   dsChar,dsString,dsComment,dsOthers};

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
  : attr(attribute), ctx(context) {
}

HlItemWw::HlItemWw(int attribute, int context)
  : HlItem(attribute,context) {
}


HlCharDetect::HlCharDetect(int attribute, int context, QChar c)
  : HlItem(attribute,context), sChar(c) {
}

const QChar *HlCharDetect::checkHgl(const QChar *str) {
  if (*str == sChar) return str + 1;
  return 0L;
}

Hl2CharDetect::Hl2CharDetect(int attribute, int context, QChar ch1, QChar ch2)
  : HlItem(attribute,context) {
  sChar1 = ch1;
  sChar2 = ch2;
}
#ifdef PASCAL_SUPPORT
Hl2CharDetect::Hl2CharDetect(int attribute, int context, const QChar *s)
  : HlItem(attribute,context) {
  sChar1 = s[0];
  sChar2 = s[1];
}
#endif

const QChar *Hl2CharDetect::checkHgl(const QChar *str) {
  if (str[0] == sChar1 && str[1] == sChar2) return str + 2;
  return 0L;
}

HlStringDetect::HlStringDetect(int attribute, int context, const QString &s)
  : HlItem(attribute, context), str(s) {
}

HlStringDetect::~HlStringDetect() {
}

const QChar *HlStringDetect::checkHgl(const QChar *s) {
  if (memcmp(s, str.unicode(), str.length()*sizeof(QChar)) == 0) return s + str.length();
  return 0L;
}


HlRangeDetect::HlRangeDetect(int attribute, int context, QChar ch1, QChar ch2)
  : HlItem(attribute,context) {
  sChar1 = ch1;
  sChar2 = ch2;
}

const QChar *HlRangeDetect::checkHgl(const QChar *s) {
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
HlKeyword::HlKeyword(int attribute, int context)
  : HlItemWw(attribute,context) {
//  words.setAutoDelete(true);
	Dict.resize(83);
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
 words+=list;
 for(uint i=0;i<list.count();i++) Dict.insert(list[i],"dummy");
}

void HlKeyword::addList(const char **list) {

  while (*list) {
    words.append(*list);
    Dict.insert(*list,"dummy");
    list++;
  }
}

const QChar *HlKeyword::checkHgl(const QChar *s) {
#if 0
  for (QStringList::Iterator it = words.begin(); it != words.end(); ++it) {
    if (memcmp(s, (*it).unicode(), (*it).length()*sizeof(QChar)) == 0) {
      return s + (*it).length();
    }
  }
	return 0L;
#else
// this seems to speed up the lookup of keywords somewhat
// anyway it has to be better than iterating through the list

#ifdef STATS
	static bool once=false;
	if(!once) Dict.statistics();
	once=true;
#endif
  const QChar *s2=s;
  while( !ustrchr("!%&()*+,-./:;<=>?[]^{|}~ ", *s2) && *s2 != '\0') s2++;
  QString lookup=QString(s,s2-s)+QString::null;
  return Dict[lookup] ? s2 : 0L;

#endif
}


HlInt::HlInt(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlInt::checkHgl(const QChar *str) {
  const QChar *s;

  s = str;
  while (s->isDigit()) s++;
  if (s > str) return s;
  return 0L;
}

HlFloat::HlFloat(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlFloat::checkHgl(const QChar *s) {
  bool b, p;

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
  if ((*s&0xdf) == 'E') s++; else return (p) ? s : 0L;
  if (*s == '-') s++;
  b = false;
  while (s->isDigit()) {
    s++;
    b = true;
  }
  if (b) return s; else return 0L;
}


HlCInt::HlCInt(int attribute, int context)
  : HlInt(attribute,context) {
}

const QChar *HlCInt::checkHgl(const QChar *s) {

//  if (*s == '0') s++; else s = HlInt::checkHgl(s);
  s = HlInt::checkHgl(s);
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

const QChar *HlCOct::checkHgl(const QChar *str) {
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

const QChar *HlCHex::checkHgl(const QChar *str) {
  const QChar *s;

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
}

HlCFloat::HlCFloat(int attribute, int context)
  : HlFloat(attribute,context) {
}

const QChar *HlCFloat::checkHgl(const QChar *s) {

  s = HlFloat::checkHgl(s);
  if (s && ((*s&0xdf) == 'F' )) s++;
  return s;
}

HlCSymbol::HlCSymbol(int attribute, int context)
  : HlItem(attribute, context) {
}

const QChar *HlCSymbol::checkHgl(const QChar *s) {
  if (ustrchr("!%&()*+,-./:;<=>?[]^{|}~", *s)) return s +1;
  return 0L;
}


HlLineContinue::HlLineContinue(int attribute, int context)
  : HlItem(attribute,context) {
}

const QChar *HlLineContinue::checkHgl(const QChar *s) {
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

const QChar *HlCStringChar::checkHgl(const QChar *str) {
  return checkEscapedChar(str);
}


HlCChar::HlCChar(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlCChar::checkHgl(const QChar *str) {
  const QChar *s;

  if (str[0] == '\'' && str[1] != '\0' && str[1] != '\'') {
    s = checkEscapedChar(&str[1]); //try to match escaped char
    if (!s) s = &str[2];           //match single non-escaped char
    if (*s == '\'') return s + 1;
  }
  return 0L;
}

HlCPrep::HlCPrep(int attribute, int context)
  : HlItem(attribute,context) {
}

const QChar *HlCPrep::checkHgl(const QChar *s) {

  while (*s == ' ' || *s == '\t') s++;
  if (*s == '#') {
    s++;
    return s;
  }
  return 0L;
}

HlHtmlTag::HlHtmlTag(int attribute, int context)
  : HlItem(attribute,context) {
}

const QChar *HlHtmlTag::checkHgl(const QChar *s) {
  while (*s == ' ' || *s == '\t') s++;
  while (*s != ' ' && *s != '\t' && *s != '>' && *s != '\0') s++;
  return s;
}

HlHtmlValue::HlHtmlValue(int attribute, int context)
  : HlItem(attribute,context) {
}

const QChar *HlHtmlValue::checkHgl(const QChar *s) {
  while (*s == ' ' || *s == '\t') s++;
  if (*s == '\"') {
    do {
      s++;
      if (*s == '\0') return 0L;
    } while (*s != '\"');
    s++;
  } else {
    while (*s != ' ' && *s != '\t' && *s != '>' && *s != '\0') s++;
  }
  return s;
}

HlShellComment::HlShellComment(int attribute, int context)
  : HlCharDetect(attribute,context,'#') {
}

HlMHex::HlMHex(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlMHex::checkHgl(const QChar *s) {

  if (s->isDigit()) {
    s++;
    while ((s->isDigit()) || (*s >= 'A' && *s <= 'F')) s++;
    if (*s == 'H') return s + 1;
  }
  return 0L;
}

HlAdaDec::HlAdaDec(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlAdaDec::checkHgl(const QChar *s) {
  const QChar *str;

  if (s->isDigit()) {
    s++;
    while ((s->isDigit()) || *s == '_') s++;
    if ((*s&0xdf) != 'E') return s;
    s++;
    str = s;
    while ((s->isDigit()) || *s == '_') s++;
    if (s > str) return s;
  }
  return 0L;
}

HlAdaBaseN::HlAdaBaseN(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlAdaBaseN::checkHgl(const QChar *s) {
  int base;
  QChar c1, c2, c3;
  const QChar *str;

  base = 0;
  while (s->isDigit()) {
    base *= 10;
    base += *s - '0';
    if (base > 16) return 0L;
    s++;
  }
  if (base >= 2 && *s == '#') {
    s++;
    c1 = '0' + ((base <= 10) ? base : 10);
    c2 = 'A' + base - 10;
    c3 = 'a' + base - 10;
    while ((*s >= '0' && *s < c1) || (*s >= 'A' && *s < c2)
      || (*s >= 'a' && *s < c3) || *s == '_') {
      s++;
    }
    if (*s == '#') {
      s++;
      if ((*s&0xdf) != 'E') return s;
      s++;
      str = s;
      while ((s->isDigit()) || *s == '_') s++;
      if (s > str) return s;
    }
  }
  return 0L;
}

HlAdaFloat::HlAdaFloat(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlAdaFloat::checkHgl(const QChar *s) {
  const QChar *str;

  str = s;
  while (s->isDigit()) s++;
  if (s > str && *s == '.') {
    s++;
    str = s;
    while (s->isDigit()) s++;
    if (s > str) {
      if ((*s&0xdf) != 'E') return s;
      s++;
      if (*s == '-') s++;
      str = s;
      while ((s->isDigit()) || *s == '_') s++;
      if (s > str) return s;
    }
  }
  return 0L;
}

HlAdaChar::HlAdaChar(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlAdaChar::checkHgl(const QChar *s) {
  if (s[0] == '\'' && s[1] && s[2] == '\'') return s + 3;
  return 0L;
}

HlSatherClassname::HlSatherClassname(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlSatherClassname::checkHgl(const QChar *s) {
  if (*s == '$') s++;

  if (*s >= 'A' && *s <= 'Z') {
    s++;
    while ((*s >= 'A' && *s <= 'Z')
           || (s->isDigit())
           || *s == '_') s++;
    return s;
  }
  return 0L;
}

HlSatherIdent::HlSatherIdent(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlSatherIdent::checkHgl(const QChar *s) {
  if (s->isLetter()) {
    s++;
                while(isInWord(*s)) s++;
    if (*s == '!') s++;
    return s;
  }
  return 0L;
}

HlSatherDec::HlSatherDec(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlSatherDec::checkHgl(const QChar *s) {
  if (s->isDigit()) {
    s++;
    while ((s->isDigit()) || *s == '_') s++;
    if (*s == 'i') s++;
    return s;
  }
  return 0L;
}

HlSatherBaseN::HlSatherBaseN(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlSatherBaseN::checkHgl(const QChar *s) {
  if (*s == '0') {
    s++;
    if (*s == 'x') {
      s++;
      while ((s->isDigit())
//           || (*s >= 'a' && *s <= 'f')
             || ((*s&0xdf) >= 'A' && (*s&0xdf) <= 'F')
             || *s == '_') s++;
    } else if (*s == 'o') {
      s++;
      while ((*s >= '0' && *s <= '7') || *s == '_') s++;
    } else if (*s == 'b') {
      s++;
      while (*s == '0' || *s == '1' || *s == '_') s++;
    } else
      return 0L;
    if (*s == 'i') s++;
    return s;
  }
  return 0L;
}

HlSatherFloat::HlSatherFloat(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlSatherFloat::checkHgl(const QChar *s) {
  if (s->isDigit()) {
    s++;
    while ((s->isDigit()) || *s == '_') s++;
    if (*s == '.') {
      s++;
      while (s->isDigit()) s++;
      if ((*s&0xdf) == 'E') {
        s++;
        if (*s == '-') s++;
        if (s->isDigit()) {
          s++;
          while ((s->isDigit()) || *s == '_') s++;
        } else
          return 0L;
      }
      if (*s == 'i') return s+1;
      if (*s == 'd') s++;
      if (*s == 'x') s++;
                // "dx" is allowed too
      return s;
    }
  }
  return 0L;
}

HlSatherChar::HlSatherChar(int attribute, int context)
  : HlItemWw(attribute,context) {
}

const QChar *HlSatherChar::checkHgl(const QChar *s) {
  if (*s == '\'') {
    s++;
    if (*s == '\\') {
      s++;
      if (*s == 'a' || *s == 'b' || *s == 'f' || *s == 'n'
          || *s == 'r' || *s == 't' || *s == 'v' || *s == '\\'
          || *s == '\'' || *s == '\"') s++;
      else if (*s>='0' && *s<='7')
        while (*s>='0' && *s<='7') s++;
      else
        return 0L;
    } else if (*s != '\0') s++;
  }
  if (*s == '\'')
    return s+1;
  else
    return 0L;
}

HlSatherString::HlSatherString(int attribute, int context)
  : HlItemWw(attribute, context) {
}

const QChar *HlSatherString::checkHgl(const QChar *s) {
  if (*s == '\"') {
    s++;
    while (*s != '\"') {
      if (*s == '\\')
        s++;
      if (*s == '\n' || *s == '\0')
        return s;
      s++;
    }
    return s+1;
  }
  return 0L;
}


HlLatexTag::HlLatexTag(int attribute, int context)
  : HlItem(attribute, context) {
}

const QChar *HlLatexTag::checkHgl(const QChar *s) {
  const QChar *str;

  if (*s == '\\') {
    s++;
    if (*s == ' ' || *s == '/' || *s == '\\') return s +1;
    str = s;
    while (((*s&0xdf) >= 'A' && (*s&0xdf) <= 'Z')
      || (s->isDigit()) || *s == '@') {
      s++;
    }
    if (s != str) return s;
  } /*else if (*s == '$') return s +1*/;
  return 0L;
}

HlLatexChar::HlLatexChar(int attribute, int context)
  : HlItem(attribute, context) {
}

const QChar *HlLatexChar::checkHgl(const QChar *s) {
  if (*s == '\\') {
    s++;
    if (*s && strchr("{}$&#_%", *s)) return s +1;
  }/* else if (*s == '"') {
    s++;
    if (*s && (*s < '0' || *s > '9')) return s +1;
  } */
  return 0L;
}

HlLatexParam::HlLatexParam(int attribute, int context)
  : HlItem(attribute, context) {
}

const QChar *HlLatexParam::checkHgl(const QChar *s) {
  if (*s == '#') {
    s++;
    while (s->isDigit()) {
      s++;
    }
    return s;
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

ItemFont::ItemFont() : family("courier"), size(12), charset("") {
}

ItemData::ItemData(const char * name, int defStyleNum)
  : name(name), defStyleNum(defStyleNum), defStyle(true), defFont(true) {
}

ItemData::ItemData(const char * name, int defStyleNum,
  const QColor &col, const QColor &selCol, bool bold, bool italic)
  : ItemStyle(col,selCol,bold,italic), name(name), defStyleNum(defStyleNum),
  defStyle(false), defFont(true) {
}

HlData::HlData(const QString &wildcards, const QString &mimetypes)
  : wildcards(wildcards), mimetypes(mimetypes) {

  itemDataList.setAutoDelete(true);
}

Highlight::Highlight(const char * name) : iName(name), refCount(0) {
}

Highlight::~Highlight() {
}

KConfig *Highlight::getKConfig() {
  KConfig *config;

  config = KWriteFactory::instance()->config();
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
    config->readEntry("Mimetypes", iMimetypes));
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
  char family[96];
  char charset[48];

  list.clear();
  list.setAutoDelete(true);
  createItemData(list);

  for (p = list.first(); p != 0L; p = list.next()) {
    s = config->readEntry(p->name);
    if (!s.isEmpty()) {
      sscanf(s.ascii(),"%d,%X,%X,%d,%d,%d,%95[^,],%d,%47[^,]",
        &p->defStyle,&col,&selCol,&p->bold,&p->italic,
        &p->defFont,family,&p->size,charset);
      p->col.setRgb(col);
      p->selCol.setRgb(selCol);
      p->family = family;
      p->charset = charset;
    }
  }
}

void Highlight::setItemDataList(ItemDataList &list, KConfig *config) {
  ItemData *p;
  QString s;

  for (p = list.first(); p != 0L; p = list.next()) {
    s.sprintf("%d,%X,%X,%d,%d,%d,%1.95s,%d,%1.47s",
      p->defStyle,p->col.rgb(),p->selCol.rgb(),p->bold,p->italic,
      p->defFont,p->family.utf8().data(),p->size,p->charset.utf8().data());
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

  s1 = str;
  while (*s1 != '\0') {
    for (item = context->items.first(); item != 0L; item = context->items.next()) {
      if (item->startEnable(lastChar)) {
        s2 = item->checkHgl(s1);
        if (s2 > s1) {
          if (item->endEnable(*s2)) {
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


CHighlight::CHighlight(const char * name) : GenHighlight(name) {
  iWildcards = "*.c";
  iMimetypes = "text/x-c-src";
}

CHighlight::~CHighlight() {
}

void CHighlight::createItemData(ItemDataList &list) {

  list.append(new ItemData(I18N_NOOP("Normal Text" ),dsNormal));
  list.append(new ItemData(I18N_NOOP("Keyword"     ),dsKeyword));
  list.append(new ItemData(I18N_NOOP("Data Type"   ),dsDataType));
  list.append(new ItemData(I18N_NOOP("Decimal"     ),dsDecVal));
  list.append(new ItemData(I18N_NOOP("Octal"       ),dsBaseN));
  list.append(new ItemData(I18N_NOOP("Hex"         ),dsBaseN));
  list.append(new ItemData(I18N_NOOP("Float"       ),dsFloat));
  list.append(new ItemData(I18N_NOOP("Char"        ),dsChar));
  list.append(new ItemData(I18N_NOOP("String"      ),dsString));
  list.append(new ItemData(I18N_NOOP("String Char" ),dsChar));
  list.append(new ItemData(I18N_NOOP("Comment"     ),dsComment));
  list.append(new ItemData(I18N_NOOP("Symbol"      ),dsNormal));
  list.append(new ItemData(I18N_NOOP("Preprocessor"),dsOthers));
  list.append(new ItemData(I18N_NOOP("Prep. Lib"   ),dsOthers,Qt::darkYellow,Qt::yellow,false,false));

}

void CHighlight::makeContextList() {
  HlContext *c;
  HlKeyword *keyword, *dataType;

  //normal context
  contextList[0] = c = new HlContext(0,0);
  c->items.append(keyword = new HlKeyword(1,0));
  c->items.append(dataType = new HlKeyword(2,0));
  c->items.append(new HlCFloat(6,0));
  c->items.append(new HlCOct(4,0));
  c->items.append(new HlCHex(5,0));
  c->items.append(new HlCInt(3,0));
  c->items.append(new HlCChar(7,0));
  c->items.append(new HlCharDetect(8,1,'"'));
  c->items.append(new Hl2CharDetect(10,2, '/', '/'));
  c->items.append(new Hl2CharDetect(10,3, '/', '*'));
  c->items.append(new HlCSymbol(11,0));
  c->items.append(new HlCPrep(12,4));
  //string context
  contextList[1] = c = new HlContext(8,0);
  c->items.append(new HlLineContinue(8,6));
  c->items.append(new HlCStringChar(9,1));
  c->items.append(new HlCharDetect(8,0,'"'));
  //one line comment context
  contextList[2] = new HlContext(10,0);
  //multi line comment context
  contextList[3] = c = new HlContext(10,3);
  c->items.append(new Hl2CharDetect(10,0, '*', '/'));
  //preprocessor context
  contextList[4] = c = new HlContext(12,0);
  c->items.append(new HlLineContinue(12,7));
  c->items.append(new HlRangeDetect(13,4, '\"', '\"'));
  c->items.append(new HlRangeDetect(13,4, '<', '>'));
  c->items.append(new Hl2CharDetect(10,2, '/', '/'));
  c->items.append(new Hl2CharDetect(10,5, '/', '*'));
  //preprocessor multiline comment context
  contextList[5] = c = new HlContext(10,5);
  c->items.append(new Hl2CharDetect(10,4, '*', '/'));
  //string line continue
  contextList[6] = new HlContext(0,1);
  //preprocessor string line continue
  contextList[7] = new HlContext(0,4);

  setKeywords(keyword, dataType);
}

void getKeywords(QStringList& configlist,QString name) {
	KConfig *config = KWriteFactory::instance()->config();
	config->setGroup(QString::fromUtf8("%1 Highlight").arg(name) );
	configlist = config->readListEntry(QString::fromUtf8("%1Keywords").arg(name) );
}

void getTypes(QStringList& configlist,QString name) {
	KConfig *config = KWriteFactory::instance()->config();
	config->setGroup(QString::fromUtf8("%1 Highlight").arg(name));
	configlist = config->readListEntry(QString::fromUtf8("%1Type").arg(name) );
}


void CHighlight::setKeywords(HlKeyword *keyword, HlKeyword *dataType) {

   KConfig *config = KWriteFactory::instance()->config();
   QStringList configlist;
   getKeywords(configlist,this->name());
// read config file for list of keywords/data types
// if there is none isEmpty() == True  then process normally and save it in config file
// if there is a list already in config file use that instead

  if(configlist.isEmpty()) {
    keyword->addList(cKeywords);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
   	config->writeEntry(QString::fromUtf8("%1Keywords").arg(name()) ,keyword->getList());
  }
  else
    keyword->addList(configlist);

  configlist.clear();
  getTypes(configlist,this->name());

  if(configlist.isEmpty()) {
    dataType->addList(cTypes);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
  	config->writeEntry(QString::fromUtf8("%1Types").arg(name()) ,dataType->getList());
 	}
  else
     dataType->addList(configlist);
}

CppHighlight::CppHighlight(const char * name) : CHighlight(name) {

  iWildcards = "*.cpp;*.cc;*.C;*.h";
  iMimetypes = "text/x-c++-src;text/x-c++-hdr;text/x-c-hdr;text/x-c++-src";
}

CppHighlight::~CppHighlight() {
}

void CppHighlight::setKeywords(HlKeyword *keyword, HlKeyword *dataType) {

  KConfig *config = KWriteFactory::instance()->config();
  config->setGroup(QString("%1 Highlight").arg(name()));


  QStringList configlist;
  getKeywords(configlist,"C");

  if ( configlist.isEmpty() ) {
    keyword->addList( cKeywords );
    config->setGroup(QString::fromUtf8("C Highlight") );
    config->writeEntry(QString::fromUtf8("CKeywords"),keyword->getList());
  } else
    keyword->addList(configlist);


  configlist.clear();
//  config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
//  configlist=config->readListEntry(QString::fromUtf8("%1Keywords").arg(name()) );
  getKeywords(configlist,name());
  if (configlist.isEmpty()) {
    keyword->addList(cppKeywords);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1Keywords").arg(name()) , keyword->getList());
  }else
    keyword->addList(configlist);

  configlist.clear();
  config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
  getTypes(configlist,"C");
  if (configlist.isEmpty()) {
    dataType->addList(cTypes);
    config->setGroup(QString::fromUtf8("C Highlight") );
    config->writeEntry(QString::fromUtf8("CTypes"),dataType->getList());
  }else
    dataType->addList(configlist);

	configlist.clear();
//  config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
//  configlist=config->readListEntry(QString::fromUtf8("%1Types").arg(name() ));
    getTypes(configlist,name());
	if (configlist.isEmpty()) {
    dataType->addList(cppTypes);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1Types").arg(name() ),dataType->getList());
  }
  else
    dataType->addList(configlist);
}

ObjcHighlight::ObjcHighlight(const char * name) : CHighlight(name) {
  iWildcards = "*.m;*.h";
  iMimetypes = "text/x-objc-src;text/x-c-hdr";
}

ObjcHighlight::~ObjcHighlight() {
}

void ObjcHighlight::makeContextList() {
  HlContext *c;

  CHighlight::makeContextList();
  c = contextList[0];
  c->items.append(new Hl2CharDetect(8,1,'@','"'));
}
// UNTESTED
void ObjcHighlight::setKeywords(HlKeyword *keyword, HlKeyword *dataType) {

  KConfig *config = KWriteFactory::instance()->config();
// config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
  QStringList configlist;
//  configlist=config->readListEntry(QString::fromUtf8("%1Keywords").arg(name() ));
  getKeywords(configlist,"C");

  if(configlist.isEmpty()) {
	  keyword->addList(cKeywords);
	  config->setGroup(QString::fromUtf8("C Highlight"));
	  config->writeEntry(QString::fromUtf8("CKeywords"),keyword->getList());
	}else
	  keyword->addList(configlist);

  configlist.clear();
//  configlist=config->readListEntry(QString::fromUtf8("%1Keywords").arg(name() ));
  getKeywords(configlist,name());
  if(configlist.isEmpty()) {
	  keyword->addList(objcKeywords);
	  config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
	  config->writeEntry(QString::fromUtf8("%1Keywords").arg(name()),keyword->getList());
	}else
	  keyword->addList(configlist);

  //configlist=config->readListEntry(QString::fromUtf8("%1Types").arg(name() ));
  getKeywords(configlist,"C");
  if(configlist.isEmpty()) {
	  dataType->addList(cTypes);
	  config->setGroup(QString::fromUtf8("C Highlight"));
	  config->writeEntry(QString::fromUtf8("CTypes"),dataType->getList());
	}else
	  keyword->addList(configlist);

  configlist.clear();
  //configlist=config->readListEntry(QString::fromUtf8("%1Types").arg(name() ));
  getTypes(configlist,name());
	if(configlist.isEmpty()) {
	  dataType->addList(objcTypes);
	  config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
	  config->writeEntry(QString::fromUtf8("%1Types").arg(name()),dataType->getList());
	}else
	  keyword->addList(configlist);

}
#ifdef PASCAL_SUPPORT
HlCaseInsensitiveKeyword::HlCaseInsensitiveKeyword(int attribute, int context)
  : HlKeyword(attribute,context) {
}

HlCaseInsensitiveKeyword::~HlCaseInsensitiveKeyword() {
}

const QChar *HlCaseInsensitiveKeyword::checkHgl(const QChar *s)
{
  const QChar *s2=s;
  while (!s2->isSpace() && *s2 != '\0'&& *s2 != ';' ) s2++; // find space
  QString lookup=QString(s,s2-s)+QString::null;
  return Dict[lookup.lower()] ? s2 : 0L;
}
const char *HlCaseInsensitiveKeyword::checkHgl(const char *s) {
  int z, count;
  QString word;

  count = words.count();
  for (z = 0; z < count; z++) {
    word = *words.at(z);
    if (strncasecmp(s,word.latin1(),word.length()) == 0) {
      return s + word.length();
    }
  }
  return 0L;
}

PascalHighlight::PascalHighlight(const char *name) : GenHighlight(name) {
  iWildcards = "*.pp;*.pas;*.inc";
  iMimetypes = "text/x-pascal-src";
}

PascalHighlight::~PascalHighlight() {
}

void PascalHighlight::createItemData(ItemDataList &list) {
  list.append(new ItemData("Normal Text",dsNormal));   // 0
  list.append(new ItemData("Keyword",dsKeyword));      // 1
  list.append(new ItemData("Data Type",dsDataType));   // 2
  list.append(new ItemData("Number",dsDecVal));        // 3
  list.append(new ItemData("String",dsString));        // 4
  list.append(new ItemData("Directive",dsOthers));     // 5
  list.append(new ItemData("Comment",dsComment));      // 6
}

void PascalHighlight::makeContextList() {
  HlContext *c;
  HlKeyword *keyword, *dataType;

  contextList[0] = c = new HlContext(0,0);
    c->items.append(keyword = new HlCaseInsensitiveKeyword(1,0));
    c->items.append(dataType = new HlCaseInsensitiveKeyword(2,0));
    c->items.append(new HlFloat(3,0));
    c->items.append(new HlInt(3,0));
    // TODO: Pascal hex $1234
    c->items.append(new HlCharDetect(4,1,'\''));
    c->items.append(new HlStringDetect(5,2,"(*$"));
    c->items.append(new Hl2CharDetect(5,3,(QChar*)"{$"));
    c->items.append(new Hl2CharDetect(6,4,(QChar*) "(*"));
    c->items.append(new HlCharDetect(6,5,'{'));
    c->items.append(new Hl2CharDetect(6,6,(QChar*) "//"));

  // string context
  contextList[1] = c = new HlContext(4,0);
    c->items.append(new HlCharDetect(4,0,'\''));
  // TODO: detect '''' or 'Holger''s Jokes are silly'

  // (*$ directive context
  contextList[2] = c = new HlContext(5,2);
    c->items.append(new Hl2CharDetect(5,0,(QChar*)"*)"));
  // {$ directive context
  contextList[3] = c = new HlContext(5,3);
    c->items.append(new HlCharDetect(5,0,'}'));
  // (* comment context
  contextList[4] = c = new HlContext(6,4);
    c->items.append(new Hl2CharDetect(6,0,(QChar*)"*)"));
  // { comment context
  contextList[5] = c = new HlContext(6,5);
    c->items.append(new HlCharDetect(6,0,'}'));
  // one line context
  contextList[6] = c = new HlContext(6,0);

  QStringList configlist;
  KConfig *config=KWriteFactory::instance()->config();
  getKeywords(configlist,name());
  if(configlist.isEmpty()) {
    keyword->addList(pascalKeywords);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1Keywords").arg(name()),keyword->getList());
  } else
    keyword->addList(configlist);

  configlist.clear();
  getTypes(configlist,name());

  if(configlist.isEmpty()) {
    dataType->addList(pascalKeywords);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1Keywords").arg(name()),dataType->getList());
  } else
  dataType->addList(pascalTypes);
}
#endif

IdlHighlight::IdlHighlight(const char * name) : CHighlight(name) {
  iWildcards = "*.idl";
  iMimetypes = "text/x-idl-src";
}


IdlHighlight::~IdlHighlight() {
}

void IdlHighlight::setKeywords(HlKeyword *keyword, HlKeyword *dataType) {

  KConfig *config = KWriteFactory::instance()->config();
  //config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
  QStringList configlist;
  //configlist=config->readListEntry(QString::fromUtf8("%1Keywords").arg(name() ));
    getKeywords(configlist,name());
	if(configlist.isEmpty()) {
	keyword->addList(idlKeywords);
	config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
	config->writeEntry(QString::fromUtf8("%1Keywords").arg(name()),keyword->getList());
	}
	else
    keyword->addList(configlist);

  configlist.clear();
  //configlist=config->readListEntry(QString::fromUtf8("%1Types").arg(name() ));
  getTypes(configlist,name());
  if (configlist.isEmpty()) {
    dataType->addList(idlTypes);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1Types").arg(name()),dataType->getList());
  }
  else
  dataType->addList(configlist);
}

JavaHighlight::JavaHighlight(const char * name) : CHighlight(name) {
  iWildcards = "*.java";
  iMimetypes = "text/x-java-src";
}

JavaHighlight::~JavaHighlight() {
}

// UNTESTED
void JavaHighlight::setKeywords(HlKeyword *keyword, HlKeyword *dataType)
{
  KConfig *config = KWriteFactory::instance()->config();
//	config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
  QStringList configlist;
//  configlist=config->readListEntry(QString::fromUtf8("%1Keywords").arg(name() ));
    getKeywords(configlist,name());

  if(configlist.isEmpty()) {
    keyword->addList(javaKeywords);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1Keywords").arg(name()),keyword->getList());
  }
  else
  keyword->addList(configlist);

  configlist.clear();
//  configlist=config->readListEntry(QString::fromUtf8("%1Types").arg(name() ));
  getTypes(configlist,name());
  if (configlist.isEmpty()) {
    dataType->addList(javaTypes);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1Types").arg(name()),dataType->getList());
  }
  else dataType->addList(configlist);
}


HtmlHighlight::HtmlHighlight(const char * name) : GenHighlight(name) {
  iWildcards = "*.html;*.htm";
  iMimetypes = "text/html";
}

HtmlHighlight::~HtmlHighlight() {
}

void HtmlHighlight::createItemData(ItemDataList &list) {

  list.append(new ItemData(I18N_NOOP("Normal Text"),dsNormal));
  list.append(new ItemData(I18N_NOOP("Char"       ),dsChar,Qt::darkGreen,Qt::green,false,false));
  list.append(new ItemData(I18N_NOOP("Comment"    ),dsComment));
  list.append(new ItemData(I18N_NOOP("Tag Text"   ),dsOthers,Qt::black,Qt::white,true,false));
  list.append(new ItemData(I18N_NOOP("Tag"        ),dsKeyword,Qt::darkMagenta,Qt::magenta,true,false));
  list.append(new ItemData(I18N_NOOP("Tag Value"  ),dsDecVal,Qt::darkCyan,Qt::cyan,false,false));
}

void HtmlHighlight::makeContextList() {
  HlContext *c;

  contextList[0] = c = new HlContext(0,0);
    c->items.append(new HlRangeDetect(1,0, '&', ';'));
    c->items.append(new HlStringDetect(2,1,"<!--"));
    c->items.append(new HlStringDetect(2,2,"<COMMENT>"));
    c->items.append(new HlCharDetect(3,3,'<'));
  contextList[1] = c = new HlContext(2,1);
    c->items.append(new HlStringDetect(2,0,"-->"));
  contextList[2] = c = new HlContext(2,2);
    c->items.append(new HlStringDetect(2,0,"</COMMENT>"));
  contextList[3] = c = new HlContext(3,3);
    c->items.append(new HlHtmlTag(4,3));
    c->items.append(new HlHtmlValue(5,3));
    c->items.append(new HlCharDetect(3,0,'>'));
}


BashHighlight::BashHighlight(const char * name) : GenHighlight(name) {
//  iWildcards = "";
  iMimetypes = "text/x-shellscript";
}



BashHighlight::~BashHighlight() {
}

void BashHighlight::createItemData(ItemDataList &list) {

  list.append(new ItemData(I18N_NOOP("Normal Text" ),dsNormal));
  list.append(new ItemData(I18N_NOOP("Keyword"     ),dsKeyword));
  list.append(new ItemData(I18N_NOOP("Integer"     ),dsDecVal));
  list.append(new ItemData(I18N_NOOP("String"      ),dsString));
  list.append(new ItemData(I18N_NOOP("Substitution"),dsOthers));//darkCyan,cyan,false,false);
  list.append(new ItemData(I18N_NOOP("Comment"     ),dsComment));
}

void BashHighlight::makeContextList() {
  HlContext *c;
  HlKeyword *keyword;

  contextList[0] = c = new HlContext(0,0);
    c->items.append(keyword = new HlKeyword(1,0));
    c->items.append(new HlInt(2,0));
    c->items.append(new HlCharDetect(3,1,'"'));
    c->items.append(new HlCharDetect(4,2,'`'));
    c->items.append(new HlShellComment(5,3));
  contextList[1] = c = new HlContext(3,0);
    c->items.append(new HlCharDetect(3,0,'"'));
  contextList[2] = c = new HlContext(4,0);
    c->items.append(new HlCharDetect(4,0,'`'));
  contextList[3] = new HlContext(5,0);

  KConfig *config = KWriteFactory::instance()->config();
//config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
  QStringList configlist;
//  configlist=config->readListEntry(QString::fromUtf8("%1Keywords").arg(name() ));
  getKeywords(configlist,name());
  if(configlist.isEmpty()) {
    keyword->addList(bashKeywords);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1Keywords").arg(name()),keyword->getList());
  }
  else
    keyword->addList(configlist);
}


ModulaHighlight::ModulaHighlight(const char * name) : GenHighlight(name) {
  iWildcards = "*.md;*.mi";
  iMimetypes = "text/x-modula-2-src";
}

ModulaHighlight::~ModulaHighlight() {
}

void ModulaHighlight::createItemData(ItemDataList &list) {

  list.append(new ItemData(I18N_NOOP("Normal Text"),dsNormal));
  list.append(new ItemData(I18N_NOOP("Keyword"    ),dsKeyword));
  list.append(new ItemData(I18N_NOOP("Decimal"    ),dsDecVal));
  list.append(new ItemData(I18N_NOOP("Hex"        ),dsBaseN));
  list.append(new ItemData(I18N_NOOP("Float"      ),dsFloat));
  list.append(new ItemData(I18N_NOOP("String"     ),dsString));
  list.append(new ItemData(I18N_NOOP("Comment"    ),dsComment));
}
// UNTESTED
void ModulaHighlight::makeContextList() {
  HlContext *c;
  HlKeyword *keyword;

  contextList[0] = c = new HlContext(0,0);
    c->items.append(keyword = new HlKeyword(1,0));
    c->items.append(new HlFloat(4,0));
    c->items.append(new HlMHex(3,0));
    c->items.append(new HlInt(2,0));
    c->items.append(new HlCharDetect(5,1,'"'));
    c->items.append(new Hl2CharDetect(6,2, '(', '*'));
  contextList[1] = c = new HlContext(5,0);
    c->items.append(new HlCharDetect(5,0,'"'));
  contextList[2] = c = new HlContext(6,2);
    c->items.append(new Hl2CharDetect(6,0, '*', ')'));

  KConfig *config = KWriteFactory::instance()->config();
//config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
  QStringList configlist;
//  configlist=config->readListEntry(QString::fromUtf8("%1Keywords").arg(name() ));
    getKeywords(configlist,name());
  if(configlist.isEmpty()) {
    keyword->addList(modulaKeywords);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1Keywords").arg(name()),keyword->getList());
  }
  else
  keyword->addList(configlist);
}


AdaHighlight::AdaHighlight(const char * name) : GenHighlight(name) {
  iWildcards = "*.a";
  iMimetypes = "text/x-ada-src";
}

AdaHighlight::~AdaHighlight() {
}

void AdaHighlight::createItemData(ItemDataList &list) {

  list.append(new ItemData(I18N_NOOP("Normal Text"),dsNormal));
  list.append(new ItemData(I18N_NOOP("Keyword"    ),dsKeyword));
  list.append(new ItemData(I18N_NOOP("Decimal"    ),dsDecVal));
  list.append(new ItemData(I18N_NOOP("Base-N"     ),dsBaseN));
  list.append(new ItemData(I18N_NOOP("Float"      ),dsFloat));
  list.append(new ItemData(I18N_NOOP("Char"       ),dsChar));
  list.append(new ItemData(I18N_NOOP("String"     ),dsString));
  list.append(new ItemData(I18N_NOOP("Comment"    ),dsComment));
}
// UNTESTED
void AdaHighlight::makeContextList() {
  HlContext *c;
  HlKeyword *keyword;

  contextList[0] = c = new HlContext(0,0);
    c->items.append(keyword = new HlKeyword(1,0));
    c->items.append(new HlAdaBaseN(3,0));
    c->items.append(new HlAdaFloat(4,0));
    c->items.append(new HlAdaDec(2,0));
    c->items.append(new HlAdaChar(5,0));
    c->items.append(new HlCharDetect(6,1,'"'));
    c->items.append(new Hl2CharDetect(7,2, '-', '-'));
  contextList[1] = c = new HlContext(6,0);
    c->items.append(new HlCharDetect(6,0,'"'));
  contextList[2] = c = new HlContext(7,0);

  KConfig *config = KWriteFactory::instance()->config();
//config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
  QStringList configlist;
//  configlist=config->readListEntry(QString::fromUtf8("%1Keywords").arg(name() ));
  getKeywords(configlist,name());
  if(configlist.isEmpty()) {
    keyword->addList(bashKeywords);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1Keywords").arg(name()),keyword->getList());
  } else
    keyword->addList(configlist);
}


PythonHighlight::PythonHighlight(const char * name) : GenHighlight(name) {
  iWildcards = "*.py";
  iMimetypes = "text/x-python-src";
}

PythonHighlight::~PythonHighlight() {
}

void PythonHighlight::createItemData(ItemDataList &list) {

  list.append(new ItemData(I18N_NOOP("Normal Text"),dsNormal));
  list.append(new ItemData(I18N_NOOP("Keyword"    ),dsKeyword));
  list.append(new ItemData(I18N_NOOP("Decimal"    ),dsDecVal));
  list.append(new ItemData(I18N_NOOP("Octal"      ),dsBaseN));
  list.append(new ItemData(I18N_NOOP("Hex"        ),dsBaseN));
  list.append(new ItemData(I18N_NOOP("Float"      ),dsFloat));
  list.append(new ItemData(I18N_NOOP("Char"       ),dsChar));
  list.append(new ItemData(I18N_NOOP("String"     ),dsString));
  list.append(new ItemData(I18N_NOOP("String Char"),dsChar));
  list.append(new ItemData(I18N_NOOP("Comment"    ),dsComment));
}

void PythonHighlight::makeContextList() {
  HlContext *c;
  HlKeyword *keyword;

  //note that a C octal has to be detected before an int and """ before "
  contextList[0] = c = new HlContext(0,0);
    c->items.append(keyword = new HlKeyword(1,0));
    c->items.append(new HlCOct(3,0));
    c->items.append(new HlInt(2,0));
    c->items.append(new HlCHex(4,0));
    c->items.append(new HlFloat(5,0));
    c->items.append(new HlCChar(6,0));
    c->items.append(new HlStringDetect(7,3,"\"\"\""));

    c->items.append(new HlStringDetect(7,4,"\'\'\'"));
    c->items.append(new HlCharDetect(7,1,'"'));
    c->items.append(new HlCharDetect(7,2,'\''));
    c->items.append(new HlCharDetect(9,5,'#'));
  contextList[1] = c = new HlContext(7,0);
    c->items.append(new HlLineContinue(7,6));
    c->items.append(new HlCStringChar(8,1));
    c->items.append(new HlCharDetect(7,0,'"'));
  contextList[2] = c = new HlContext(7,0);
    c->items.append(new HlLineContinue(7,7));
    c->items.append(new HlCStringChar(8,2));
    c->items.append(new HlCharDetect(7,0,'\''));
  contextList[3] = c = new HlContext(7,3);
    c->items.append(new HlStringDetect(7,0,"\"\"\""));
  contextList[4] = c = new HlContext(7,4);
    c->items.append(new HlStringDetect(7,0,"\'\'\'"));
  contextList[5] = new HlContext(9,0);
  contextList[6] = new HlContext(0,1);
  contextList[7] = new HlContext(0,2);

  KConfig *config = KWriteFactory::instance()->config();
//	config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
  QStringList configlist;
//  configlist=config->readListEntry(QString::fromUtf8("%1Keywords").arg(name() ));
  getKeywords(configlist,name());
  if(configlist.isEmpty()) {
    keyword->addList(pythonKeywords);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1Keywords").arg(name()),keyword->getList());
  } else
    keyword->addList(configlist);

}

PerlHighlight::PerlHighlight(const char * name) : Highlight(name) {
  iWildcards = "";
  iMimetypes = "application/x-perl";
}

void PerlHighlight::createItemData(ItemDataList &list) {

  list.append(new ItemData(I18N_NOOP("Normal Text"),dsNormal));
  list.append(new ItemData(I18N_NOOP("Keyword"    ),dsKeyword));
  list.append(new ItemData(I18N_NOOP("Variable"   ),dsDecVal));
  list.append(new ItemData(I18N_NOOP("Operator"   ),dsOthers));
  list.append(new ItemData(I18N_NOOP("String"     ),dsString));
  list.append(new ItemData(I18N_NOOP("String Char"),dsChar));
  list.append(new ItemData(I18N_NOOP("Comment"    ),dsComment));
  list.append(new ItemData(I18N_NOOP("Pod"        ),dsOthers, Qt::darkYellow, Qt::yellow, false, true));
}



/*
hardcoded perl highlight

Op Customary  Generic     Meaning    Interpolates         Modifiers
1     ''       q{}       Literal         no
2     ""      qq{}       Literal         yes
3     ``      qx{}       Command         yes (no for ')
4             qw{}      Word list        no
5     //       m{}    Pattern match      yes (no for ')   cgimosx
6              s{}{}   Substitution      yes (no for ')   egimosx
7             tr{}{}   Translation       no               cds
7              y{}{}   Translation       no               cds
*/
int PerlHighlight::doHighlight(int ctxNum, TextLine *textLine) {
  static const char *opList[] = {"q", "qq", "qx", "qw", "m", "s", "tr", "y"};
  static int opLenList[] = {1, 2, 2, 2, 1, 1, 2, 1}; // length of strings in opList
  QChar delimiter;
  bool division;
  int op, argCount;
  bool interpolating, brackets, pod;

  const QChar *str, *s, *s2;
  QChar lastChar;
  bool lastWw;
  int pos, z, l;

  //extract some states out of the context number
  delimiter = QChar(ctxNum >> 9);
  division = ctxNum & 256;
  op = (ctxNum >> 5) & 7;
  argCount = (ctxNum >> 3) & 3;
  interpolating = !(ctxNum & 4);
  brackets = ctxNum & 2;
  pod = ctxNum & 1;

  //current line to process
  str = textLine->getString();
  //last caracter and its whole word check status
  lastChar = ' ';
  lastWw = true;

  s = str;

  //match pod documentation tags
  if (*s == '=') {
    s++;
    pod = true;
    if (ucmp(s, "cut", 3)) {
      pod = false;
      s += 3;
      textLine->setAttribs(7, 0, 4);
    }
  }
  if (pod) {
    textLine->setAttribs(7, 0, textLine->length());
    textLine->setAttr(7);
    goto finished;
  }
  while (*s) {
    pos = s - str;
    if (op == 0 && lastWw) {
      //match keyword
      s2 = keyword->checkHgl(s);
      if (s2 && !isInWord(*s2)) {
        s = s2;
        textLine->setAttribs(1, pos, s - str);
        goto newContext;
      }
      //match perl operator
      if (lastChar != '-') {
        for (z = 0; z < 8; z++) {
          l = opLenList[z];
          if (ucmp(s, opList[z], l) && !isInWord(s[l])) {
            //operator found
            op = z;            // generate op number (1 to 7)
            if (op < 7) op++;
            argCount = (op >= 6) ? 2 : 1; // number of arguments
            s += l;
            textLine->setAttribs(3, pos, pos + l);
            goto newContext;
          }
        }
      }
      //match customary
      if (*s == '\'') {
        op = 1;
        interpolating = false;
      }
      if (*s == '"') {
        op = 2;
      }
      if (*s == '`') {
        op = 3;
      }
      if (!division && *s == '/') { // don't take it if / is division
        op = 5;
      }
      if (op != 0) {
        delimiter = *s;
        s++;
        argCount = 1;
        textLine->setAttribs(3, pos, pos + 1);
        goto newContext;
      }
    }
    if (delimiter == '\0') { //not in string
      // match comment
      if (lastWw && *s == '#') {
        textLine->setAttribs(6, pos, textLine->length());
        textLine->setAttr(6);
        goto finished;
      }
      // match delimiter
      if (op != 0 && !s->isSpace()) {
        delimiter = *s;
        if (delimiter == '(') {
          delimiter = ')';
          brackets = true;
        }
        if (delimiter == '<') {
          delimiter = '>';
          brackets = true;
        }
        if (delimiter == '[') {
          delimiter = ']';
          brackets = true;
        }
        if (delimiter == '{') {
          delimiter = '}';
          brackets = true;
        }
        s++;
        if (op == 1 || op == 4 || op == 7 || (delimiter == '\'' && op != 2))
          interpolating = false;
        textLine->setAttribs(3, pos, pos + 1);
        goto newContext;
      }
      // match bind operator or command end
      if (*s == '~' || *s == ';') {
        division = false; // pattern matches with / now allowed
      }
    }
    if (interpolating) {
      // match variable
      if (*s == '$' || *s == '@' || *s == '%') {
        s2 = s;
        do {
          s2++;
        } while ((isInWord(*s2) || *s2 == '#') && *s2 != delimiter);
        if (s2 - s > 1) {
          // variable found
          s = s2;
          textLine->setAttribs(2, pos, s2 - str);
          division = true; // division / or /= may follow
          goto newContext;
        }
      }
      // match special variables
      if (s[0] == '$' && s[1] != '\0' && s[1] != delimiter) {
        if (ustrchr("&`'+*./|,\\;#%=-~^:?!@$<>()[]", s[1])) {
          // special variable found
          s += 2;
          textLine->setAttribs(2, pos, pos + 2);
          division = true; // division / or /= may follow
          goto newContext;
        }
      }
    }
    if (delimiter != '\0') { //in string
      //match escaped char
      if (interpolating) {
        if (*s == '\\' && s[1] != '\0') {
          s++;
          s2 = checkCharHexOct(s);
          if (s2) s = s2; else s++;
          textLine->setAttribs(5, pos, s - str);
          goto newContext;
        }
      }
      //match string end
      if (delimiter == *s) {
        s++;
        argCount--;
        if (argCount < 1) {
          //match operator modifiers
          if (op == 5) while (*s && ustrchr("cgimosx", *s)) s++;
          if (op == 6) while (*s && ustrchr("egimosx", *s)) s++;
          if (op == 7) while (*s && ustrchr("cds", *s)) s++;
          op = 0;
        }
        textLine->setAttribs(3, pos, s - str);
        if (brackets || op == 0) {
          interpolating = true;
          delimiter = '\0'; //string end delimiter = '\0' means "not in string"
          brackets = false;
        }
      } else {
        //highlight a ordinary character in string
        s++;
        textLine->setAttribs(4, pos, pos + 1);
      }
      goto newContext;
    }
    s++;
    textLine->setAttribs(0, pos, pos + 1);
    newContext:
    lastChar = s[-1];
    lastWw = !isInWord(lastChar);
  }
  textLine->setAttr(0);
  finished:

  //compose new context number
  ctxNum = delimiter.unicode() << 9;
  if (division) ctxNum |= 256;
  ctxNum |= op << 5;
  ctxNum |= argCount << 3;
  if (!interpolating) ctxNum |= 4;
  if (brackets) ctxNum |= 2;
  if (pod) ctxNum |= 1;
  return ctxNum;
  //method will be called again if there are more lines to highlight
}

void PerlHighlight::init() {
  keyword = new HlKeyword(0,0);

  KConfig *config = KWriteFactory::instance()->config();
//config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
  QStringList configlist;
//  configlist=config->readListEntry(QString::fromUtf8("%1Keywords").arg(name() ));
  getKeywords(configlist,name());
  if(configlist.isEmpty()) {
    keyword->addList(perlKeywords);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1Keywords").arg(name()),keyword->getList());
  }
  else
    keyword->addList(configlist);
//  keyword->addList(perlKeywords);
}

void PerlHighlight::done() {
  delete keyword;
}

SatherHighlight::SatherHighlight(const char * name) : GenHighlight(name) {
  iWildcards = "*.sa";
  iMimetypes = "text/x-sather-src";
}

SatherHighlight::~SatherHighlight() {
}

void SatherHighlight::createItemData(ItemDataList &list) {

  list.append(new ItemData(I18N_NOOP("Normal Text"        ),dsNormal)); // 0
  list.append(new ItemData(I18N_NOOP("Keyword"            ),dsKeyword));// 1
  list.append(new ItemData(I18N_NOOP("Special Classname"  ), dsNormal));// 2
  list.append(new ItemData(I18N_NOOP("Classname"          ),dsNormal)); // 3
  list.append(new ItemData(I18N_NOOP("Special Featurename"),dsOthers)); // 4
  list.append(new ItemData(I18N_NOOP("Identifier"         ),dsOthers)); // 5
  list.append(new ItemData(I18N_NOOP("Decimal"            ),dsDecVal)); // 6
  list.append(new ItemData(I18N_NOOP("Base-N"             ),dsBaseN));  // 7
  list.append(new ItemData(I18N_NOOP("Float"              ),dsFloat));  // 8
  list.append(new ItemData(I18N_NOOP("Char"               ),dsChar));   // 9
  list.append(new ItemData(I18N_NOOP("String"             ),dsString)); // 10
  list.append(new ItemData(I18N_NOOP("Comment"            ),dsComment));// 11
}
// UNTESTED
void SatherHighlight::makeContextList() {
  HlContext *c;
  HlKeyword *keyword,*spec_class,*spec_feat;

  //Normal Context
  contextList[0] = c = new HlContext(0,0);
    c->items.append(keyword = new HlKeyword(1,0));
    c->items.append(spec_class = new HlKeyword(2,0));
    c->items.append(new HlSatherClassname(3,0));
    c->items.append(spec_feat = new HlKeyword(4,0));
    c->items.append(new HlSatherIdent(5,0));
    c->items.append(new HlSatherFloat(8,0)); // check float before int
    c->items.append(new HlSatherBaseN(7,0));
    c->items.append(new HlSatherDec(6,0));
    c->items.append(new HlSatherChar(9,0));
    c->items.append(new HlSatherString(10,0));
    c->items.append(new Hl2CharDetect(11,1, '-', '-'));
  //Comment Context
  contextList[1] = c = new HlContext(11,0);

  KConfig *config = KWriteFactory::instance()->config();
//	config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
  QStringList configlist;
//  configlist=config->readListEntry(QString::fromUtf8("%1Keywords").arg(name() ));
  getKeywords(configlist,name());
  if(configlist.isEmpty()) {
    keyword->addList(satherKeywords);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1Keywords").arg(name()),keyword->getList());
  }
   else
    keyword->addList(configlist);

  configlist.clear();
  configlist=config->readListEntry(QString::fromUtf8("%1SpecClassNames").arg(name() ));
  if (configlist.isEmpty()) {
    spec_class->addList(satherSpecClassNames);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1SpecClassNames").arg(name()),spec_class->getList());
  }
  else
    spec_class->addList(configlist);

  configlist.clear();
  configlist=config->readListEntry(QString::fromUtf8("%1SpecFeatureNames").arg(name() ));
  if (configlist.isEmpty()) {
    spec_feat->addList(satherSpecClassNames);
    config->setGroup(QString::fromUtf8("%1 Highlight").arg(name()));
    config->writeEntry(QString::fromUtf8("%1SpecFeatureNames").arg(name()),spec_feat->getList());
  }
  else
    spec_feat->addList(configlist);

//  keyword->addList(satherKeywords);
//  spec_class->addList(satherSpecClassNames);
//  spec_feat->addList(satherSpecFeatureNames);
}

LatexHighlight::LatexHighlight(const char * name) : GenHighlight(name) {
  iWildcards = "*.tex;*.sty";
  iMimetypes = "text/x-tex";
}

LatexHighlight::~LatexHighlight() {
}

void LatexHighlight::createItemData(ItemDataList &list) {

  list.append(new ItemData(I18N_NOOP("Normal Text"), dsNormal));
  list.append(new ItemData(I18N_NOOP("Tag/Keyword"), dsKeyword));
 list.append(new ItemData(I18N_NOOP("Optional Argument"), dsKeyword));
 list.append(new ItemData(I18N_NOOP("Mandatory Argument"), dsKeyword));
 list.append(new ItemData(I18N_NOOP("Inline Maths"), dsDecVal));
  list.append(new ItemData(I18N_NOOP("Char"       ), dsChar));
  list.append(new ItemData(I18N_NOOP("Parameter"  ), dsDecVal));
  list.append(new ItemData(I18N_NOOP("Comment"    ), dsComment));
}

void LatexHighlight::makeContextList() {
  HlContext *c;
/*
  //normal context
  contextList[0] = c = new HlContext(0,0);
    c->items.append(new HlLatexTag(1,0));
    c->items.append(new HlLatexChar(2,0));
    c->items.append(new HlLatexParam(3,0));
    c->items.append(new HlCharDetect(4,1,'%'));
  //one line comment context
  contextList[1] = new HlContext(4,0);
*/

  //normal context
  contextList[0] = c = new HlContext(0,0);
    c->items.append(new HlLatexTag(1,4));
    c->items.append(new HlLatexChar(5,0));
    c->items.append(new HlCharDetect(4,2,'$'));
    c->items.append(new HlLatexParam(6,0));
    c->items.append(new HlCharDetect(7,1,'%'));
  //one line comment context
  contextList[1] = new HlContext(7,0);
  //multiline inline maths context
  contextList[2] = c = new HlContext(4,2);
    c->items.append(new HlLatexChar(4,2));
    c->items.append(new HlCharDetect(4,3,'%'));
    c->items.append(new HlCharDetect(4,0,'$'));
  //comments in math mode context
  contextList[3] = new HlContext(7,2);
  //arguments to functions context
  contextList[4] = c = new HlContext(0,0);
    c->items.append(new HlCharDetect(2,5, '['));
    c->items.append(new HlCharDetect(3,6, '{'));
    c->items.append(new HlLatexTag(1,4));
    c->items.append(new HlLatexChar(5,0));
    c->items.append(new HlCharDetect(4,2,'$'));
    c->items.append(new HlLatexParam(6,0));
    c->items.append(new HlCharDetect(7,1,'%'));
  //optional arguments to functions context
  //this is buggy because nested arguments can span lines
  //should be OK for 99% of the time
  contextList[5] = c = new HlContext(2,5);
    c->items.append(new HlRangeDetect(2,5, '[', ']'));
    c->items.append(new HlCharDetect(2,4, ']'));
  //mandatory arguments to functions context
  //this is buggy because nested arguments can span lines
  //should be OK for 99% of the time
  contextList[6] = c = new HlContext(3,6);
    c->items.append(new HlRangeDetect(3,6, '{', '}'));
    c->items.append(new HlCharDetect(3,4, '}'));
}

//--------

HlManager *HlManager::s_pSelf = 0;

HlManager::HlManager() : QObject(0L) {

  hlList.setAutoDelete(true);
  hlList.append(new Highlight(I18N_NOOP("Normal")));
  hlList.append(new CHighlight(     "C"        ));
  hlList.append(new CppHighlight(   "C++"      ));
  hlList.append(new ObjcHighlight(  "Objective-C"));
  hlList.append(new JavaHighlight(  "Java"     ));
  hlList.append(new HtmlHighlight(  "HTML"     ));
  hlList.append(new BashHighlight(  "Bash"     ));
  hlList.append(new ModulaHighlight("Modula 2" ));
  hlList.append(new AdaHighlight(   "Ada"      ));
#ifdef PASCAL_SUPPORT
  hlList.append(new PascalHighlight("Pascal"   ));
#endif
  hlList.append(new PythonHighlight("Python"   ));
  hlList.append(new PerlHighlight(  "Perl"     ));
  hlList.append(new SatherHighlight("Sather"   ));
  hlList.append(new LatexHighlight( "Latex"    ));
  hlList.append(new IdlHighlight("IDL"));
}

HlManager::~HlManager() {
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

  config = KWriteFactory::instance()->config();
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
  ItemFont defaultFont;
  ItemDataList itemDataList;
  ItemData *itemData;
  int nAttribs, z;
  QFont font;

  defaultStyleList.setAutoDelete(true);
  getDefaults(defaultStyleList, defaultFont);

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
      font.setBold(defaultStyle->bold);
      font.setItalic(defaultStyle->italic);
    } else {
      // custom style
      a[z].col = itemData->col;
      a[z].selCol = itemData->selCol;
      font.setBold(itemData->bold);
      font.setItalic(itemData->italic);
    }
    if (itemData->defFont) {
      font.setFamily(defaultFont.family);
      font.setPointSize(defaultFont.size);
//      KCharset(defaultFont.charset).setQFont(font);
    } else {
      font.setFamily(itemData->family);
      font.setPointSize(itemData->size);
//      KCharset(itemData->charset).setQFont(font);
    }
    a[z].setFont(font);
  }
  for (; z < maxAttribs; z++) {
    a[z].col = black;
    a[z].selCol = black;
    a[z].setFont(font);
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

void HlManager::getDefaults(ItemStyleList &list, ItemFont &font) {
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

  config = KWriteFactory::instance()->config();
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

  config->setGroup("Default Font");
  QFont defaultFont = KGlobalSettings::fixedFont();
  font.family = config->readEntry("Family", defaultFont.family());
//  qDebug("family == %s", font.family.ascii());
  font.size = config->readNumEntry("Size", defaultFont.pointSize());
//  qDebug("size == %d", font.size);
  font.charset = config->readEntry("Charset","ISO-8859-1");
}

void HlManager::setDefaults(ItemStyleList &list, ItemFont &font) {
  KConfig *config;
  int z;
  ItemStyle *i;
  char s[64];

  config = KWriteFactory::instance()->config();
  config->setGroup("Default Item Styles");
  for (z = 0; z < defaultStyles(); z++) {
    i = list.at(z);
    sprintf(s,"%X,%X,%d,%d",i->col.rgb(),i->selCol.rgb(),i->bold, i->italic);
    config->writeEntry(defaultStyleName(z),s);
  }

  config->setGroup("Default Font");
  config->writeEntry("Family",font.family);
  config->writeEntry("Size",font.size);
  config->writeEntry("Charset",font.charset);

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

StyleChanger::StyleChanger( QWidget *parent )
  : QWidget(parent)
{
  QLabel *label;

  QGridLayout *glay = new QGridLayout( this, 4, 3, 0, KDialog::spacingHint() );
  CHECK_PTR(glay);
  glay->addColSpacing( 1, KDialog::spacingHint() ); // Looks better
  glay->setColStretch( 2, 10 );

  col = new KColorButton(this);
  CHECK_PTR(col);
  connect(col,SIGNAL(changed(const QColor &)),this,SLOT(changed()));
  label = new QLabel(col,i18n("Normal:"),this);
  CHECK_PTR(label);
  glay->addWidget(label,0,0);
  glay->addWidget(col,1,0);

  selCol = new KColorButton(this);
  CHECK_PTR(selCol);
  connect(selCol,SIGNAL(changed(const QColor &)),this,SLOT(changed()));
  label = new QLabel(selCol,i18n("Selected:"),this);
  CHECK_PTR(label);
  glay->addWidget(label,2,0);
  glay->addWidget(selCol,3,0);

  bold = new QCheckBox(i18n("Bold"),this);
  CHECK_PTR(bold);
  connect(bold,SIGNAL(clicked()),SLOT(changed()));
  glay->addWidget(bold,1,2);

  italic = new QCheckBox(i18n("Italic"),this);
  CHECK_PTR(italic);
  connect(italic,SIGNAL(clicked()),SLOT(changed()));
  glay->addWidget(italic,2,2);
}

void StyleChanger::setRef(ItemStyle *s) {

  style = s;
  col->setColor(style->col);
  selCol->setColor(style->selCol);
  bold->setChecked(style->bold);
  italic->setChecked(style->italic);

}

void StyleChanger::setEnabled(bool enable) {

  col->setEnabled(enable);
  selCol->setEnabled(enable);
  bold->setEnabled(enable);
  italic->setEnabled(enable);
}

void StyleChanger::changed() {

  if (style) {
    style->col = col->color();
    style->selCol = selCol->color();
    style->bold = bold->isChecked();
    style->italic = italic->isChecked();
  }
}





FontChanger::FontChanger( QWidget *parent )
  : QWidget(parent)
{
  QLabel *label;
  QStringList fontList;

  QVBoxLayout *vlay = new QVBoxLayout( this, 0, KDialog::spacingHint() );
  CHECK_PTR(vlay);

  familyCombo = new QComboBox(true,this);
  CHECK_PTR(familyCombo);
  label = new QLabel( familyCombo,i18n("Family:"), this );
  CHECK_PTR(label);
  vlay->addWidget(label);
  vlay->addWidget(familyCombo);
  connect( familyCombo, SIGNAL(activated(const QString&)),
           this, SLOT(familyChanged(const QString&)));
  KFontChooser::getFontList(fontList, false);
  familyCombo->insertStringList(fontList);


  sizeCombo = new QComboBox(true,this);
  CHECK_PTR(sizeCombo);
  label = new QLabel(sizeCombo,i18n("Size:"),this);
  CHECK_PTR(label);
  vlay->addWidget(label);
  vlay->addWidget(sizeCombo);
  connect( sizeCombo, SIGNAL(activated(int)),
           this, SLOT(sizeChanged(int)) );
  for( int i=0; fontSizes[i] != 0; i++ ){
    sizeCombo->insertItem(QString().setNum(fontSizes[i]));
  }


  charsetCombo = new QComboBox(true,this);
  CHECK_PTR(charsetCombo);
  label = new QLabel(charsetCombo,i18n("Charset:"),this);
  CHECK_PTR(label);
  vlay->addWidget(label);
  vlay->addWidget(charsetCombo);
  connect( charsetCombo, SIGNAL(activated(const QString&)),
           this, SLOT(charsetChanged(const QString&)) );
}


void FontChanger::setRef(ItemFont *f) {
  int z;

  font = f;
  for (z = 0; z < (int) familyCombo->count(); z++) {
    if (font->family == familyCombo->text(z)) {
      familyCombo->setCurrentItem(z);
      goto found;
    }
  }
  font->family = familyCombo->text(0);
found:

  for (z = 0; fontSizes[z] > 0; z++) {
    if (font->size == fontSizes[z]) {
      sizeCombo->setCurrentItem(z);
      break;
    }
  }
  displayCharsets();
}

void FontChanger::familyChanged(const QString& family) {

  font->family = family;
  displayCharsets();
}

void FontChanger::sizeChanged(int n) {

  font->size = fontSizes[n];;
}

void FontChanger::charsetChanged(const QString& charset) {

  font->charset = charset;
  //KCharset(chset).setQFont(font);
}

void FontChanger::displayCharsets() {
  int z;
  QString charset;
  KCharsets *charsets;

  charsets = KGlobal::charsets();
  QStringList lst = charsets->availableCharsetNames(font->family);
//  QStrList lst = charsets->displayable(font->family);
  charsetCombo->clear();
  for(z = 0; z < (int) lst.count(); z++) {
    charset = *lst.at(z);
    charsetCombo->insertItem(charset);
    if (/*(QString)*/ font->charset == charset) charsetCombo->setCurrentItem(z);
  }
  charset = "any";
  charsetCombo->insertItem(charset);
  if (/*(QString)*/ font->charset == charset) charsetCombo->setCurrentItem(z);
}

//---------


HighlightDialog::HighlightDialog( HlManager *hlManager, ItemStyleList *styleList,
                                  ItemFont *font,
                                  HlDataList *highlightDataList,
                                  int hlNumber, QWidget *parent,
                                  const char *name, bool modal )
  :KDialogBase(KDialogBase::Tabbed, i18n("Highlight Settings"), Ok|Cancel, Ok, parent, name, modal),
   defaultItemStyleList(styleList), hlData(0L)
{

  // defaults =========================================================

  QFrame *page1 = addPage(i18n("&Defaults"));
  QGridLayout *grid = new QGridLayout(page1,2,2,0,spacingHint());

  QVGroupBox *dvbox1 = new QVGroupBox( i18n("Default Item Styles"), page1 );
  /*QLabel *label = */new QLabel( i18n("Item:"), dvbox1 );
  QComboBox *styleCombo = new QComboBox( false, dvbox1 );
  defaultStyleChanger = new StyleChanger( dvbox1 );
  for( int i = 0; i < hlManager->defaultStyles(); i++ ) {
    styleCombo->insertItem(i18n(hlManager->defaultStyleName(i)));
  }
  connect(styleCombo, SIGNAL(activated(int)), this, SLOT(defaultChanged(int)));
  grid->addWidget(dvbox1,0,0);

  QVGroupBox *dvbox2 = new QVGroupBox( i18n("Default Font"), page1 );
  defaultFontChanger = new FontChanger( dvbox2 );
  defaultFontChanger->setRef(font);
  grid->addWidget(dvbox2,0,1);

  grid->setRowStretch(1,1);
  grid->setColStretch(1,1);

  defaultChanged(0);

  // highlight modes =====================================================

  QFrame *page2 = addPage(i18n("&Highlight Modes"));
  grid = new QGridLayout(page2,3,2,0,spacingHint());

  QVGroupBox *vbox1 = new QVGroupBox( i18n("Config Select"), page2 );
  grid->addWidget(vbox1,0,0);
  QVGroupBox *vbox2 = new QVGroupBox( i18n("Item Style"), page2 );
  grid->addWidget(vbox2,1,0);
  QVGroupBox *vbox3 = new QVGroupBox( i18n("Highlight Auto Select"), page2 );
  grid->addWidget(vbox3,0,1);
  QVGroupBox *vbox4 = new QVGroupBox( i18n("Item Font"), page2 );
  grid->addWidget(vbox4,1,1);

  grid->setRowStretch(2,1);
  grid->setColStretch(1,1);

  QLabel *label = new QLabel( i18n("Highlight:"), vbox1 );
  hlCombo = new QComboBox( false, vbox1 );
  connect( hlCombo, SIGNAL(activated(int)),
           this, SLOT(hlChanged(int)) );
  for( int i = 0; i < hlManager->highlights(); i++) {
    hlCombo->insertItem(hlManager->hlName(i));
  }
  hlCombo->setCurrentItem(hlNumber);


  label = new QLabel( i18n("Item:"), vbox1 );
  itemCombo = new QComboBox( false, vbox1 );
  connect( itemCombo, SIGNAL(activated(int)), this, SLOT(itemChanged(int)) );

  label = new QLabel( i18n("File Extensions:"), vbox3 );
  wildcards  = new QLineEdit( vbox3 );
  label = new QLabel( i18n("Mime Types:"), vbox3 );
  mimetypes = new QLineEdit( vbox3 );


  styleDefault = new QCheckBox(i18n("Default"), vbox2 );
  connect(styleDefault,SIGNAL(clicked()),SLOT(changed()));
  styleChanger = new StyleChanger( vbox2 );


  fontDefault = new QCheckBox(i18n("Default"), vbox4 );
  connect(fontDefault,SIGNAL(clicked()),SLOT(changed()));
  fontChanger = new FontChanger( vbox4 );

  hlDataList = highlightDataList;
  hlChanged(hlNumber);
}


void HighlightDialog::defaultChanged(int z)
{
  defaultStyleChanger->setRef(defaultItemStyleList->at(z));
}


void HighlightDialog::hlChanged(int z)
{
  writeback();

  hlData = hlDataList->at(z);

  wildcards->setText(hlData->wildcards);
  mimetypes->setText(hlData->mimetypes);

  itemCombo->clear();
  for (ItemData *itemData = hlData->itemDataList.first(); itemData != 0L;
    itemData = hlData->itemDataList.next()) {
    itemCombo->insertItem(i18n(itemData->name));
  }

  itemChanged(0);
}

void HighlightDialog::itemChanged(int z)
{
  itemData = hlData->itemDataList.at(z);

  styleDefault->setChecked(itemData->defStyle);
  styleChanger->setRef(itemData);

  fontDefault->setChecked(itemData->defFont);
  fontChanger->setRef(itemData);
}

void HighlightDialog::changed()
{
  itemData->defStyle = styleDefault->isChecked();
  itemData->defFont = fontDefault->isChecked();
}

void HighlightDialog::writeback() {
  if (hlData) {
    hlData->wildcards = wildcards->text();
    hlData->mimetypes = mimetypes->text();
  }
}

void HighlightDialog::done(int r) {
  writeback();
  QDialog::done(r);
}

#include "highlight.moc"
