/* This file is part of the KDE project
   Copyright (C) 2002 Christoph Cullmann <cullmann@kde.org>

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

#include "kateprojectmanager.h"
#include "kateprojectmanager.moc"

#include "kateapp.h"
#include "katemainwindow.h"

#include <kconfig.h>
#include <kcombobox.h>
#include <kdialogbase.h>
#include <kurlrequester.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>

#include <qfile.h>
#include <qlayout.h>
#include <qlabel.h>

KateProjectManager::KateProjectManager (QObject *parent) : QObject (parent)
{
  m_projects.setAutoDelete (true);
  m_projectManager = new Kate::ProjectManager (this);
  setupPluginList ();
}

KateProjectManager::~KateProjectManager()
{
}

void KateProjectManager::setupPluginList ()
{
  QValueList<KService::Ptr> traderList= KTrader::self()->query("Kate/ProjectPlugin");

  KTrader::OfferList::Iterator it(traderList.begin());
  for( ; it != traderList.end(); ++it)
  {
    KService::Ptr ptr = (*it);

    QString pVersion = ptr->property("X-Kate-Version").toString();
    
    if ((pVersion >= "2.2") && (pVersion <= KATE_VERSION))
    {
      ProjectPluginInfo *info=new ProjectPluginInfo;
  
      info->service = ptr;
      info->name=info->service->library();
  
      info->projectType=info->service->property("X-Kate-ProjectType").toString();
  
      m_pluginList.append(info);
    }
  }
}

void KateProjectManager::setCurrentProject (Kate::Project *project)
{
  m_currentProject = project;

  emit m_projectManager->projectChanged ();
}

Kate::Project *KateProjectManager::create (const QString &type, const QString &name, const QString &filename)
{
  KConfig *c = new KConfig (filename, false, false);

  c->setGroup("Project File");
  c->writeEntry ("Type", type);
  c->writeEntry ("Name", name);
  c->sync ();

  delete c;

  return open (filename);
}

Kate::Project *KateProjectManager::open (const QString &filename)
{
  // don't open a project 2 times
  for (uint z=0; z < m_projects.count(); z++)
    if (m_projects.at(z)->fileName() == filename)
      return 0;

  KateInternalProjectData *data = new KateInternalProjectData ();
  data->proMan = this;
  data->fileName = filename;

  Kate::Project *project = new Kate::Project ((void *) data);

  m_projects.append (project);

  emit m_projectManager->projectCreated (project);

  return project;
}

bool KateProjectManager::close (Kate::Project *project)
{
  if (project)
  {
    if (project->close())
    {
      uint id = project->projectNumber ();
      int n = m_projects.findRef (project);

      if (n >= 0)
      {
        if (Kate::pluginViewInterface(project->plugin()))
        {
          for (uint i=0; i< ((KateApp*)parent())->mainWindows(); i++)
          {
            Kate::pluginViewInterface(project->plugin())->removeView(((KateApp*)parent())->mainWindow(i));
          }
        }

        m_projects.remove (n);

        emit m_projectManager->projectDeleted (id);

        return true;
      }
    }
  }

  return false;
}

Kate::Project *KateProjectManager::project (uint n)
{
  if (n >= m_projects.count())
    return 0;

  return m_projects.at(n);
}

uint KateProjectManager::projects ()
{
  return m_projects.count ();
}

Kate::ProjectPlugin *KateProjectManager::createPlugin (Kate::Project *project)
{
  ProjectPluginInfo *def = 0;
  ProjectPluginInfo *info = 0;

  for (uint i=0; i<m_pluginList.count(); i++)
  {
    if (m_pluginList.at(i)->projectType == project->type())
    {
      info = m_pluginList.at(i);
      break;
    }
    else if (m_pluginList.at(i)->projectType == QString ("Default"))
      def = m_pluginList.at(i);
  }

  if (!info)
    info = def;

  return Kate::createProjectPlugin (QFile::encodeName(info->service->library()), project);
}

void KateProjectManager::enableProjectGUI (Kate::Project *project, KateMainWindow *win)
{
  if (!project->plugin()) return;
  if (!Kate::pluginViewInterface(project->plugin())) return;

  Kate::pluginViewInterface(project->plugin())->addView(win->mainWindow());
}

void KateProjectManager::disableProjectGUI (Kate::Project *project, KateMainWindow *win)
{
  if (!project->plugin()) return;
  if (!Kate::pluginViewInterface(project->plugin())) return;

  Kate::pluginViewInterface(project->plugin())->removeView(win->mainWindow());
}

ProjectInfo *KateProjectManager::newProjectDialog (QWidget *parent)
{
  ProjectInfo *info = 0;

  KateProjectDialogNew* dlg = new KateProjectDialogNew (parent, this);

  int n = dlg->exec();

  if (n)
  {
    info = new ProjectInfo ();
    info->type = dlg->type;
    info->name = dlg->name;
    info->fileName = dlg->fileName;
  }

  delete dlg;
  return info;
}

QStringList KateProjectManager::pluginStringList ()
{
  QStringList list;

  for (uint i=0; i<m_pluginList.count(); i++)
    list.push_back (m_pluginList.at(i)->projectType);

  return list;
}

bool KateProjectManager::queryCloseAll ()
{
  for (uint z=0; z < m_projects.count(); z++)
    if (!m_projects.at(z)->queryClose())
      return false;

  return true;
}

bool KateProjectManager::closeAll ()
{
  while (!m_projects.isEmpty())
  {
    if (!close(m_projects.at(m_projects.count()-1)))
      return false;
  }

  return true;
}

void KateProjectManager::saveProjectList (class KConfig *config)
{
  QString prevGrp=config->group();
  config->setGroup ("Open Projects");

  config->writeEntry ("Count", m_projects.count());

  for (uint z=0; z < m_projects.count(); z++)
    config->writeEntry( QString("Project %1").arg(z), m_projects.at(z)->fileName() );

  config->setGroup(prevGrp);
}

void KateProjectManager::restoreProjectList (class KConfig *config)
{
  config->setGroup ("Open Projects");

  int count = config->readNumEntry("Count");

  int i = 0;
  while ((i < count) && config->hasKey(QString("Project %1").arg(i)))
  {
    QString fn = config->readEntry( QString("Project %1").arg( i ) );

    if ( !fn.isEmpty() )
      open (fn);

    i++;
  }
}

//
// "New Project" Dialog
//

KateProjectDialogNew::KateProjectDialogNew (QWidget *parent, KateProjectManager *projectMan) : KDialogBase (parent, "project_new", true, i18n ("New Project"), KDialogBase::Ok|KDialogBase::Cancel)
{
  m_projectMan = projectMan;

  QWidget *page = new QWidget( this );
  setMainWidget(page);

  QGridLayout *grid = new QGridLayout (page, 3, 2, 0, spacingHint());

  grid->addWidget (new QLabel (i18n("Project type:"), page), 0, 0);
  m_typeCombo = new KComboBox (page);
  grid->addWidget (m_typeCombo, 0, 1);

  m_typeCombo->insertStringList (m_projectMan->pluginStringList ());

  grid->addWidget (new QLabel (i18n("Project name:"), page), 1, 0);
  m_nameEdit = new KLineEdit (page);
  grid->addWidget (m_nameEdit, 1, 1);
  connect( m_nameEdit, SIGNAL( textChanged ( const QString & )),this,SLOT(slotTextChanged()));
  grid->addWidget (new QLabel (i18n("Project file:"), page), 2, 0);
  m_urlRequester = new KURLRequester (page);
  grid->addWidget (m_urlRequester, 2, 1);
  m_nameEdit->setFocus();

  m_urlRequester->setMode (KFile::LocalOnly);
  m_urlRequester->fileDialog()->setOperationMode (KFileDialog::Saving);
  m_urlRequester->setFilter (QString ("*.kateproject|")
                             + i18n("Kate Project Files") + QString (" (*.kateproject)"));
  connect( m_urlRequester->lineEdit(), SIGNAL( textChanged ( const QString & )),this,SLOT(slotTextChanged()));
  slotTextChanged();
}

KateProjectDialogNew::~KateProjectDialogNew ()
{
}

void KateProjectDialogNew::slotTextChanged()
{
  enableButtonOK( !m_urlRequester->lineEdit()->text().isEmpty() && !m_nameEdit->text().isEmpty());
}

int KateProjectDialogNew::exec()
{
  int n = 0;

  while ((n = KDialogBase::exec()))
  {
    type = m_typeCombo->currentText ();
    name = m_nameEdit->text ();
    fileName = m_urlRequester->url ();

    if (!name.isEmpty() && !fileName.isEmpty())
      break;
    else
      KMessageBox::sorry (this, i18n ("You must enter a project name and file"));
  }
  
  if (!fileName.endsWith (".kateproject"))
    fileName.append (".kateproject");

  return n;
}
