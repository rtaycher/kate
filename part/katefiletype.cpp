/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>

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

// $Id$

#include "katefiletype.h"
#include "katefiletype.moc"

#include "katedocument.h"
#include "kateconfig.h"
#include "katedialogs.h"
#include "kateview.h"
#include "katefactory.h"

#include <kconfig.h>
#include <kmimemagic.h>
#include <kmimetype.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <knuminput.h>
#include <klocale.h>
#include <kpopupmenu.h>

#include <qregexp.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qheader.h>
#include <qhgroupbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qvbox.h>
#include <qvgroupbox.h>
#include <qwhatsthis.h>
#include <qwidgetstack.h>

KateFileTypeManager::KateFileTypeManager ()
{
  m_types.setAutoDelete (true);

  update ();
}

KateFileTypeManager::~KateFileTypeManager ()
{
}

//
// read the types from config file and update the internal list
//
void KateFileTypeManager::update ()
{
  KConfig config ("katefiletyperc", false, false);

  QStringList g (config.groupList());
  g.sort ();

  m_types.clear ();
  for (uint z=0; z < g.count(); z++)
  {
    config.setGroup (g[z]);

    KateFileType *type = new KateFileType ();

    type->number = z;
    type->name = g[z];
    type->section = config.readEntry ("Section");
    type->wildcards = config.readListEntry ("Wildcards", ';');
    type->mimetypes = config.readListEntry ("Mimetypes", ';');
    type->priority = config.readNumEntry ("Priority");
    type->varLine = config.readEntry ("Variables");

    m_types.append (type);

    kdDebug(13020) << "INIT LIST: " << type->name << endl;
  }
}

//
// save the given list to config file + update
//
void KateFileTypeManager::save (QPtrList<KateFileType> *v)
{
  KConfig config ("katefiletyperc", false, false);

  QStringList newg;
  for (uint z=0; z < v->count(); z++)
  {
    config.setGroup (v->at(z)->name);

    config.writeEntry ("Section", v->at(z)->section);
    config.writeEntry ("Wildcards", v->at(z)->wildcards, ';');
    config.writeEntry ("Mimetypes", v->at(z)->mimetypes, ';');
    config.writeEntry ("Priority", v->at(z)->priority);
    config.writeEntry ("Variables", v->at(z)->varLine);

    newg << v->at(z)->name;
  }

  QStringList g (config.groupList());

  for (uint z=0; z < g.count(); z++)
  {
    if (newg.findIndex (g[z]) == -1)
      config.deleteGroup (g[z]);
  }

  config.sync ();

  update ();
}

int KateFileTypeManager::fileType (KateDocument *doc)
{
  if (!doc)
    return -1;

  if (m_types.isEmpty())
    return -1;

  QString fileName = doc->url().prettyURL();
  int length = doc->url().prettyURL().length();

  //
  // first use the wildcards
  //
  static QStringList commonSuffixes = QStringList::split (";", ".orig;.new;~;.bak;.BAK");

  int result;
  if ((result = wildcardsFind(fileName)) != -1)
    return result;

  QString backupSuffix = KateDocumentConfig::global()->backupSuffix();
  if (fileName.endsWith(backupSuffix)) {
    if ((result = wildcardsFind(fileName.left(length - backupSuffix.length()))) != -1)
      return result;
  }

  for (QStringList::Iterator it = commonSuffixes.begin(); it != commonSuffixes.end(); ++it) {
    if (*it != backupSuffix && fileName.endsWith(*it)) {
      if ((result = wildcardsFind(fileName.left(length - (*it).length()))) != -1)
        return result;
    }
  }

  //
  // now use the KMimeType POWER ;)
  //
  const int HOWMANY = 16384;
  QByteArray buf(HOWMANY);

  int bufpos = 0;
  for (uint i=0; i < doc->numLines(); i++)
  {
    QString line = doc->textLine(i);
    int len = line.length() + 1; // space for a newline - seemingly not required by kmimemagic, but nicer for debugging.

    if (bufpos + len > HOWMANY)
      len = HOWMANY - bufpos;

    memcpy(&buf[bufpos], (line+"\n").latin1(), len);
    bufpos += len;

    if (bufpos >= HOWMANY)
      break;
  }

  int accuracy;
  KMimeType::Ptr mt = KMimeType::findByContent( buf, &accuracy );

  QPtrList<KateFileType> types;

  for (uint z=0; z < m_types.count(); z++)
  {
    if (m_types.at(z)->mimetypes.findIndex (mt->name()) > -1)
      types.append (m_types.at(z));
  }

  if ( !types.isEmpty() )
  {
    int pri = -1;
    int hl = -1;

    for (KateFileType *type = types.first(); type != 0L; type = types.next())
    {
      if (type->priority > pri)
      {
        pri = type->priority;
        hl = type->number;
      }
    }

    return hl;
  }

  return -1;
}

int KateFileTypeManager::wildcardsFind (const QString &fileName)
{
  QPtrList<KateFileType> types;

  for (uint z=0; z < m_types.count(); z++)
  {
    for( QStringList::Iterator it = m_types.at(z)->wildcards.begin(); it != m_types.at(z)->wildcards.end(); ++it )
    {
      // anders: we need to be sure to match the end of string, as eg a css file
      // would otherwise end up with the c hl
      QRegExp re(*it, true, true);
      if ( ( re.search( fileName ) > -1 ) && ( re.matchedLength() == (int)fileName.length() ) )
        types.append (m_types.at(z));
    }
  }

  if ( !types.isEmpty() )
  {
    int pri = -1;
    int hl = -1;

    for (KateFileType *type = types.first(); type != 0L; type = types.next())
    {
      if (type->priority > pri)
      {
        pri = type->priority;
        hl = type->number;
      }
    }

    return hl;
  }

  return -1;
}

const KateFileType *KateFileTypeManager::fileType (uint number)
{
  if (number < m_types.count())
    return m_types.at(number);

  return 0;
}

KateFileTypeConfigTab::KateFileTypeConfigTab( QWidget *parent )
  : Kate::ConfigPage( parent )
{
  m_types.setAutoDelete (true);
  m_lastType = 0;

  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );

  // hl chooser
  QHBox *hbHl = new QHBox( this );
  layout->add (hbHl);
  hbHl->setSpacing( KDialog::spacingHint() );
  QLabel *lHl = new QLabel( i18n("&Filetype:"), hbHl );
  typeCombo = new QComboBox( false, hbHl );
  lHl->setBuddy( typeCombo );
  connect( typeCombo, SIGNAL(activated(int)),
           this, SLOT(typeChanged(int)) );

  btndel = new QPushButton( i18n("&Delete"), hbHl );
  connect( btndel, SIGNAL(clicked()), this, SLOT(deleteType()) );

  QPushButton *btnnew = new QPushButton( i18n("&New"), hbHl );
  connect( btnnew, SIGNAL(clicked()), this, SLOT(newType()) );

  gbProps = new QGroupBox( 2, Qt::Horizontal, i18n("Properties"), this );
  layout->add (gbProps);

  // file & mime types
  QLabel *lname = new QLabel( i18n("N&ame:"), gbProps );
  name  = new QLineEdit( gbProps );
  lname->setBuddy( name );
  connect( name, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );

  // file & mime types
  QLabel *lsec = new QLabel( i18n("&Section:"), gbProps );
  section  = new QLineEdit( gbProps );
  lsec->setBuddy( section );
  connect( section, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );

  // file & mime types
  QLabel *lvar = new QLabel( i18n("&Variables:"), gbProps );
  varLine  = new QLineEdit( gbProps );
  lvar->setBuddy( varLine );
  connect( varLine, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );

  // file & mime types
  QLabel *lFileExts = new QLabel( i18n("File e&xtensions:"), gbProps );
  wildcards  = new QLineEdit( gbProps );
  lFileExts->setBuddy( wildcards );
  connect( wildcards, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );

  QLabel *lMimeTypes = new QLabel( i18n("MIME &types:"), gbProps);
  QHBox *hbMT = new QHBox (gbProps);
  mimetypes = new QLineEdit( hbMT );
  lMimeTypes->setBuddy( mimetypes );
  connect( mimetypes, SIGNAL( textChanged ( const QString & ) ), this, SLOT( slotChanged() ) );

  QToolButton *btnMTW = new QToolButton(hbMT);
  btnMTW->setIconSet(QIconSet(SmallIcon("wizard")));
  connect(btnMTW, SIGNAL(clicked()), this, SLOT(showMTDlg()));

  QLabel *lprio = new QLabel( i18n("Prio&rity:"), gbProps);
  priority = new KIntNumInput( gbProps );
  lprio->setBuddy( priority );
  connect( priority, SIGNAL( valueChanged ( int ) ), this, SLOT( slotChanged() ) );

  layout->addStretch();

  reload();
}

void KateFileTypeConfigTab::apply()
{
  save ();

  KateFactory::self()->fileTypeManager()->save(&m_types);
}

void KateFileTypeConfigTab::reload()
{
  m_types.clear();
  for (uint z=0; z < KateFactory::self()->fileTypeManager()->list()->count(); z++)
  {
    KateFileType *type = new KateFileType ();

    *type = *KateFactory::self()->fileTypeManager()->list()->at(z);

    m_types.append (type);
  }

  update ();
}

void KateFileTypeConfigTab::reset()
{
  reload ();
}

void KateFileTypeConfigTab::defaults()
{
  reload ();
}

void KateFileTypeConfigTab::update ()
{
  m_lastType = 0;

  typeCombo->clear ();

  for( uint i = 0; i < m_types.count(); i++) {
    if (m_types.at(i)->section.length() > 0)
      typeCombo->insertItem(m_types.at(i)->section + QString ("/") + m_types.at(i)->name);
    else
      typeCombo->insertItem(m_types.at(i)->name);
  }

  typeCombo->setCurrentItem (0);

  typeChanged (0);

  typeCombo->setEnabled (typeCombo->count() > 0);
}

void KateFileTypeConfigTab::deleteType ()
{
  int type = typeCombo->currentItem ();

  if ((type > -1) && ((uint)type < m_types.count()))
  {
    m_types.remove (type);
    update ();
  }
}

void KateFileTypeConfigTab::newType ()
{
  QString newN = i18n("New Filetype");

  for( uint i = 0; i < m_types.count(); i++) {
    if (m_types.at(i)->name == newN)
    {
      typeCombo->setCurrentItem (i);
      typeChanged (i);
      return;
    }
  }

  KateFileType *newT = new KateFileType ();
  newT->priority = 0;
  newT->name = newN;

  m_types.prepend (newT);

  update ();
}

void KateFileTypeConfigTab::save ()
{
  if (m_lastType)
  {
    m_lastType->name = name->text ();
    m_lastType->section = section->text ();
    m_lastType->varLine = varLine->text ();
    m_lastType->wildcards = QStringList::split (";", wildcards->text ());
    m_lastType->mimetypes = QStringList::split (";", mimetypes->text ());
    m_lastType->priority = priority->value();
  }
}

void KateFileTypeConfigTab::typeChanged (int type)
{
  save ();

  KateFileType *t = 0;

  if ((type > -1) && ((uint)type < m_types.count()))
    t = m_types.at(type);

  if (t)
  {
    gbProps->setTitle (i18n("Properties of %1").arg (typeCombo->currentText()));

    gbProps->setEnabled (true);
    btndel->setEnabled (true);

    name->setText(t->name);
    section->setText(t->section);
    varLine->setText(t->varLine);
    wildcards->setText(t->wildcards.join (";"));
    mimetypes->setText(t->mimetypes.join (";"));
    priority->setValue(t->priority);
  }
  else
  {
    gbProps->setTitle (i18n("Properties"));

    gbProps->setEnabled (false);
    btndel->setEnabled (false);

    name->clear();
    section->clear();
    varLine->clear();
    wildcards->clear();
    mimetypes->clear();
    priority->setValue(0);
  }

  m_lastType = t;
}

void KateFileTypeConfigTab::showMTDlg()
{

  QString text = i18n("Select the MimeTypes you want for this file type.\nPlease note that this will automatically edit the associated file extensions as well.");
  QStringList list = QStringList::split( QRegExp("\\s*;\\s*"), mimetypes->text() );
  KMimeTypeChooserDlg *d = new KMimeTypeChooserDlg( this, i18n("Select Mime Types"), text, list );
  if ( d->exec() == KDialogBase::Accepted ) {
    // do some checking, warn user if mime types or patterns are removed.
    // if the lists are empty, and the fields not, warn.
    wildcards->setText(d->patterns().join(";"));
    mimetypes->setText(d->mimeTypes().join(";"));
  }
}

void KateViewFileTypeAction::init()
{
  m_doc = 0;
  subMenus.setAutoDelete( true );

  popupMenu()->insertItem ( i18n("None"), this, SLOT(setType(int)), 0,  0);

  connect(popupMenu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateViewFileTypeAction::updateMenu (Kate::Document *doc)
{
  m_doc = (KateDocument *)doc;
}

void KateViewFileTypeAction::slotAboutToShow()
{
  KateDocument *doc=m_doc;
  int count = KateFactory::self()->fileTypeManager()->list()->count();

  for (int z=0; z<count; z++)
  {
    QString hlName = KateFactory::self()->fileTypeManager()->list()->at(z)->name;
    QString hlSection = KateFactory::self()->fileTypeManager()->list()->at(z)->section;

    if ( !hlSection.isEmpty() && (names.contains(hlName) < 1) )
    {
      if (subMenusName.contains(hlSection) < 1)
      {
        subMenusName << hlSection;
        QPopupMenu *menu = new QPopupMenu ();
        subMenus.append(menu);
        popupMenu()->insertItem (hlSection, menu);
      }

      int m = subMenusName.findIndex (hlSection);
      names << hlName;
      subMenus.at(m)->insertItem ( hlName, this, SLOT(setType(int)), 0,  z+1);
    }
    else if (names.contains(hlName) < 1)
    {
      names << hlName;
      popupMenu()->insertItem ( hlName, this, SLOT(setType(int)), 0,  z+1);
    }
  }

  if (!doc) return;

  for (uint i=0;i<subMenus.count();i++)
  {
    for (uint i2=0;i2<subMenus.at(i)->count();i2++)
      subMenus.at(i)->setItemChecked(subMenus.at(i)->idAt(i2),false);
  }
  popupMenu()->setItemChecked (0, false);

  if (doc->fileType() == -1)
    popupMenu()->setItemChecked (0, true);
  else
  {
    const KateFileType *t = 0;
    if ((t = KateFactory::self()->fileTypeManager()->fileType (doc->fileType())))
    {
      int i = subMenusName.findIndex (t->section);
      if (i >= 0 && subMenus.at(i))
        subMenus.at(i)->setItemChecked (doc->fileType()+1, true);
      else
        popupMenu()->setItemChecked (0, true);
    }
  }
}

void KateViewFileTypeAction::setType (int mode)
{
  KateDocument *doc=m_doc;

  if (doc)
    doc->updateFileType(mode-1, true);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
