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

// $Id$

#include "kateprojectmanager.h"
#include "kateprojectmanager.moc"

#include "kateproject.h"
#include "kateapp.h"
#include "katemainwindow.h"

#include <kconfig.h>
#include <kcombobox.h>
#include <kdialogbase.h>
#include <kurlrequester.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <qfile.h>
#include <qlayout.h> 
#include <qlabel.h>

class KateProjectDialogNew : public KDialogBase
{
  public:
    KateProjectDialogNew (QWidget *parent, KateProjectManager *projectMan);
    ~KateProjectDialogNew ();
    
    int exec();
    
  private:
    KateProjectManager *m_projectMan;

    KComboBox *m_typeCombo;
    KLineEdit *m_nameEdit;
    KURLRequester *m_urlRequester;
  
  public:
    QString type;
    QString name;
    QString fileName;
};

KateProjectManager::KateProjectManager (QObject *parent) : QObject (parent)
{
  m_projects.setAutoDelete (true);
  m_projectsR.setAutoDelete (false);
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

    ProjectPluginInfo *info=new ProjectPluginInfo;

    info->service = ptr;
    info->name=info->service->property("X-KATE-InternalName").toString();
    if (info->name.isEmpty()) info->name=info->service->library();
    
    info->projectType=info->service->property("X-KATE-ProjectType").toString();
    
    m_pluginList.append(info);
  }
}

Kate::Project *KateProjectManager::create (const QString &type, const QString &name, const QString &filename)
{
  KConfig *c = new KConfig (filename);
  c->setGroup("General");
  c->writeEntry ("Type", type);
  c->writeEntry ("Name", name);
  c->sync ();
  delete c;

  return open (filename);
}
    
Kate::Project *KateProjectManager::open (const QString &filename)
{
  KateProject *project = new KateProject (this, this, filename);
  
  m_projects.append (project);
  m_projectsR.append (project->project());
  
  emit m_projectManager->projectCreated (project->project ());
  
  return project->project();
}

bool KateProjectManager::close (Kate::Project *project)
{
  if (project)
  {
    if (project->plugin()->close())
    {
      uint id = project->projectNumber ();
      int n = m_projectsR.findRef (project);
      
      if (n >= 0)
      {
        if (Kate::pluginViewInterface(project->plugin())) 
        {
          for (uint i=0; i< ((KateApp*)parent())->mainWindows(); i++)
          {
            Kate::pluginViewInterface(project->plugin())->removeView(((KateApp*)parent())->mainWindow(i));
          }
        }
        
        m_projectsR.remove (n);
        m_projects.remove (n);
        
        emit m_projectManager->projectDeleted (id);
      
        return true;
      }
    }
  }

  return false;
}

Kate::Project *KateProjectManager::project (uint n = 0)
{
  if (n >= m_projectsR.count())
    return 0;
    
  return m_projectsR.at(n);
}
    
uint KateProjectManager::projects ()
{
  return m_projectsR.count ();
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

//
// "New Project" Dialog
//

KateProjectDialogNew::KateProjectDialogNew (QWidget *parent, KateProjectManager *projectMan) : KDialogBase (parent, "project_new", true, i18n ("New Project"), KDialogBase::Ok|KDialogBase::Cancel)
{
  m_projectMan = projectMan;
  
  QWidget *page = new QWidget( this );
  setMainWidget(page);
  
  QGridLayout *grid = new QGridLayout (page, 3, 2, 0, spacingHint());
  
  grid->addWidget (new QLabel (i18n("Project Type:"), page), 0, 0);
  m_typeCombo = new KComboBox (page);
  grid->addWidget (m_typeCombo, 0, 1);
  
  m_typeCombo->insertStringList (m_projectMan->pluginStringList ());
  
  grid->addWidget (new QLabel (i18n("Project Name:"), page), 1, 0);
  m_nameEdit = new KLineEdit (page);
  grid->addWidget (m_nameEdit, 1, 1);
  
  grid->addWidget (new QLabel (i18n("Project File:"), page), 2, 0);
  m_urlRequester = new KURLRequester (page);
  grid->addWidget (m_urlRequester, 2, 1);
  
  m_urlRequester->setMode (KFile::LocalOnly);
  m_urlRequester->setFilter (QString ("*.kate|") + i18n("Kate Project Files"));
}

KateProjectDialogNew::~KateProjectDialogNew ()
{
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

  return n;
}
