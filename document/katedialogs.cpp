#include "katedialogs.h"
#include <klocale.h>
#include <kdebug.h>
#include <qgroupbox.h>
#include <qvgroupbox.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qwidgetstack.h>
#include <qlabel.h>
#include <qlistview.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <kfontdialog.h>
#include <kcharsets.h>
#include <kglobal.h>


#include "katedialogs.moc"
 /*******************************************************************************************************************
*                                        Context Editor                                                            *
*******************************************************************************************************************/

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


char fontSizes[] = {4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,22,24,26,28,32,48,64,0};


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
  QHBox *modHl = new QHBox(vbox1);
  QPushButton *createHl=new QPushButton(i18n("New"),modHl);
  QPushButton *editHl=new QPushButton(i18n("Edit"),modHl);
  connect(editHl,SIGNAL(clicked()),this,SLOT(hlEdit()));
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
    kdDebug()<<itemData->name;
    itemCombo->insertItem(i18n(itemData->name.latin1()));
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


void HighlightDialog::hlEdit() {
  HlEditDialog diag(0,0,"hlEdit", true,hlData);
  diag.show();
}



HlEditDialog::HlEditDialog(HlManager *,QWidget *parent, const char *name, bool modal,HlData *data)
  :KDialogBase(KDialogBase::Swallow, i18n("Highlight Conditions"), Ok|Cancel, Ok, parent, name, modal)
{
  QHBox *wid=new QHBox(this);
  QVBox *lbox=new QVBox(wid);
    contextList=new KListView(lbox);
    contextList->addColumn(i18n("Syntax structure"));
    contextList->setSorting(-1);
    QPushButton *addContext=new QPushButton(i18n("New Context"),lbox);
    QVGroupBox *opt  = new QVGroupBox( i18n("Options"), wid);
    stack=new QWidgetStack(opt);
    initContextOptions(contextOptions=new QVBox(stack));
    stack->addWidget(contextOptions,HlEContext);
    initItemOptions(itemOptions=new QVBox(stack));
    stack->addWidget(itemOptions,HlEItem);
    stack->raiseWidget(HlEContext);
    setMainWidget(wid);
    if (data!=0) loadFromDocument(data);
}

void HlEditDialog::initContextOptions(QVBox *co)
{
  if( co!=0)
    {
        QHBox *tmp = new QHBox(co);
        (void) new QLabel(i18n("Description:"),tmp);
        (void) new QLineEdit(tmp);
        tmp= new QHBox(co);
        (void) new QLabel(i18n("Attribute:"),tmp);
        (void) new QComboBox(tmp);
        tmp= new QHBox(co);
        (void) new QLabel(i18n("LineEnd:"),tmp);
        (void) new QComboBox(tmp);
    }
   else
     kdDebug()<<"initContextOptions: Widget is 0"<<endl;
}

void HlEditDialog::initItemOptions(QVBox *co)
{
  if (co!=0)
    {
        QHBox *tmp = new QHBox(co);
        (void) new QLabel(i18n("Type:"),tmp);
        (void) new QComboBox(tmp);
        tmp= new QHBox(co);
        (void) new QLabel(i18n("Parameter:"),tmp);
        (void) new QLineEdit(tmp);
        tmp= new QHBox(co);
        (void) new QLabel(i18n("Attribute:"),tmp);
        (void) new QComboBox(tmp);
        (void) new QLabel(i18n("Context switch:"),tmp);
        (void) new QComboBox(tmp);
    }
  else
    kdDebug()<<"initItemOptions: Widget is 0"<<endl;
}

void HlEditDialog::loadFromDocument(HlData *hl)
{
  struct syntaxContextData *data,*datasub;
  QListViewItem *last=0,*lastsub=0;

  HlManager::self()->syntax->setIdentifier(hl->identifier);
  data=HlManager::self()->syntax->getGroupInfo("highlighting","context");
  int i=0;
  if (data)
    {
      while (HlManager::self()->syntax->nextGroup(data))
        {
        kdDebug(13010)<< "Adding context to list"<<endl;
//	  contextList->insertItem(
          last= new QListViewItem(contextList,last,
                 HlManager::self()->syntax->groupData(data,QString("name")),
                 QString("%1").arg(i),
                 HlManager::self()->syntax->groupData(data,QString("attribute")),
                 HlManager::self()->syntax->groupData(data,QString("lineEndContext"))); //);
          i++;
          int iitem=0;
          lastsub=0;//last;
          bool tmpbool;
          while (HlManager::self()->syntax->nextItem(data))
              {
                kdDebug(13010)<< "Adding item to list"<<endl;
                datasub=HlManager::self()->syntax->getSubItems(data);
                if (tmpbool=HlManager::self()->syntax->nextItem(datasub))
                  {
                    for (;tmpbool;tmpbool=HlManager::self()->syntax->nextItem(datasub))
                        lastsub=addContextItem(contextList,last,lastsub,data);
                  }
                 HlManager::self()->syntax->freeGroupInfo(datasub);
              }


	 }
       if (data) HlManager::self()->syntax->freeGroupInfo(data);
   }
}

QListViewItem *HlEditDialog::addContextItem(KListView *cL,QListViewItem *_parent,QListViewItem *prev,struct syntaxContextData *data)
  {

                QString dataname=HlManager::self()->syntax->groupItemData(data,QString("name"));
                QString attr=(HlManager::self()->syntax->groupItemData(data,QString("attribute")));
                QString context=(HlManager::self()->syntax->groupItemData(data,QString("context")));
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
                QString param("");
                if ((dataname=="keyword") || (dataname=="dataType")) param=dataname;
                  else if (dataname=="CharDetect") param=chr;
                    else if ((dataname=="2CharDetect") || (dataname=="RangeDetect")) param=chr+chr1;
                      else if ((dataname=="StringDetect") || (dataname=="AnyChar") || (dataname=="RegExpr")) param=stringdata;
                        else                     kdDebug(13010)<<"***********************************"<<endl<<"Unknown entry for Context:"<<dataname<<endl;
                kdDebug(13010)<<dataname << endl;
                return new QListViewItem(_parent,prev,i18n(dataname.latin1())+param,dataname,param,attr,context);
 }

