/* plugin_katebuild.c                    Kate Plugin
**
** Copyright (C) 2006 by Kåre Särs <kare.sars@iki.fi>
**
** This code is mostly a modification of the GPL'ed Make plugin
** by Adriaan de Groot.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program in a file called COPYING; if not, write to
** the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
** MA 02110-1301, USA.
*/

#include "plugin_katebuild.moc"

#include <cassert>

#include <qfile.h>
#include <qfileinfo.h>
#include <qinputdialog.h>
#include <qregexp.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qpalette.h>
#include <qlabel.h>
#include <QApplication>
#include <QCompleter>
#include <QDirModel>

#include <kaction.h>
#include <kactioncollection.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kpassivepopup.h>
#include <klineedit.h>
#include <kconfig.h>
#include <kfiledialog.h>



#include <kgenericfactory.h>

K_EXPORT_COMPONENT_FACTORY( katebuildplugin, KGenericFactory<KateBuildPlugin>( "katebuild-plugin" ) )

#define COL_FILE    (0)
#define COL_LINE    (1)
#define COL_MSG     (2)
#define COL_URL     (3)

#define DEF_QUICK_COMP_CMD "gcc -Wall -g %f"


/******************************************************************/
KateBuildPlugin::KateBuildPlugin(QObject *parent, const QStringList&):
        Kate::Plugin ((Kate::Application*)parent)
{
}

/******************************************************************/
Kate::PluginView *KateBuildPlugin::createView (Kate::MainWindow *mainWindow)
{
    return new KateBuildView(mainWindow);
}

/******************************************************************/
KateBuildView::KateBuildView(Kate::MainWindow *mw)
    : Kate::PluginView (mw)
     , KXMLGUIClient()
     , m_toolView (mw->createToolView ("kate_private_plugin_katebuildplugin",
                Kate::MainWindow::Bottom,
                SmallIcon("application-x-ms-dos-executable"),
                i18n("Build Output"))
               )
    , m_proc(0)
    // NOTE this will not allow spaces in file names.
    , m_filenameDetector("([a-np-zA-Z]:[\\\\/])?[a-zA-Z0-9_\\.\\-/\\\\]+\\.[chpxCHPX]+:[0-9]+"),
    m_newDirDetector("make\\[.+\\]: .+ `.*'")
{
    setComponentData(KComponentData("kate"));

    m_win=mw;

    KAction *make = actionCollection()->addAction("run_make");
    make->setText(i18n("Run make"));
    make->setShortcut(QKeySequence(Qt::ALT+Qt::Key_R) );
    connect( make, SIGNAL( triggered(bool) ), this, SLOT( slotMake() ) );

    KAction *clean = actionCollection()->addAction("make_clean");
    clean->setText(i18n("Make Clean"));
    clean->setShortcut(QKeySequence(Qt::ALT+Qt::Key_Z) );
    connect( clean, SIGNAL( triggered(bool) ), this, SLOT( slotMakeClean() ) );

    KAction *quick = actionCollection()->addAction("quick_compile");
    quick->setText(i18n("Quick Compile"));
    quick->setShortcut(QKeySequence(Qt::ALT+Qt::Key_C) );
    connect( quick, SIGNAL( triggered(bool) ), this, SLOT( slotQuickCompile() ) );

    KAction *stop = actionCollection()->addAction("stop");
    stop->setText(i18n("Stop"));
    stop->setShortcut(QKeySequence(Qt::ALT+Qt::Key_X) );
    connect( stop, SIGNAL( triggered(bool) ), this, SLOT( slotStop() ) );

    KAction *next = actionCollection()->addAction("goto_next");
    next->setText(i18n("Next Error"));
    next->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_Right) );
    connect( next, SIGNAL( triggered(bool) ), this, SLOT( slotNext() ) );

    KAction *prev = actionCollection()->addAction("goto_prev");
    prev->setText(i18n("Previous Error"));
    prev->setShortcut(QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_Left) );
    connect( prev, SIGNAL( triggered(bool) ), this, SLOT( slotPrev() ) );

    QWidget *buildWidget = new QWidget(m_toolView);
    buildUi.setupUi(buildWidget);


    connect(buildUi.errTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
            SLOT(slotItemSelected(QTreeWidgetItem *)));

    buildUi.plainTextEdit->setReadOnly(true);

    buildUi.browse->setIcon(KIcon("inode-directory"));
    connect(buildUi.browse, SIGNAL(clicked()), this, SLOT(slotBrowseClicked()));

    // set the default values of the build settings. (I think loading a plugin should also trigger
    // a read of the session config data, but it does not)
    buildUi.buildCmd->setText("make");
    buildUi.cleanCmd->setText("make clean");
    buildUi.quickComp->setText(DEF_QUICK_COMP_CMD);

    QCompleter* dirCompleter = new QCompleter(this);
    QStringList filter;
    dirCompleter->setModel(new QDirModel(filter, QDir::AllDirs|QDir::NoDotAndDotDot, QDir::Name, this));
    buildUi.buildDir->setCompleter(dirCompleter);


    
    m_proc = new KProcess();

    connect(m_proc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotProcExited(int, QProcess::ExitStatus)));
    connect(m_proc, SIGNAL(readyReadStandardError()),this, SLOT(slotReadReadyStdErr()));
    connect(m_proc, SIGNAL(readyReadStandardOutput()),this, SLOT(slotReadReadyStdOut()));
    
    setXMLFile(QString::fromLatin1("plugins/katebuild/ui.rc"));
    mainWindow()->guiFactory()->addClient(this);
}



/******************************************************************/
KateBuildView::~KateBuildView()
{

    mainWindow()->guiFactory()->removeClient( this );

    delete m_proc;
    delete m_toolView;
}

/******************************************************************/
QWidget *KateBuildView::toolView() const
{
    return m_toolView;
}

/******************************************************************/
void KateBuildView::readSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
    KConfigGroup cg(config, groupPrefix + ":build-plugin");
    buildUi.buildDir->setText(cg.readEntry("Make Path", QString()));
    buildUi.buildCmd->setText(cg.readEntry("Make Command", "make"));
    buildUi.cleanCmd->setText(cg.readEntry("Clean Command", "make clean"));
    buildUi.quickComp->setText(cg.readEntry("Quick Compile Command", DEF_QUICK_COMP_CMD));
}

/******************************************************************/
void KateBuildView::writeSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
    KConfigGroup cg(config, groupPrefix + ":build-plugin");
    cg.writeEntry("Make Path", buildUi.buildDir->text());
    cg.writeEntry("Make Command", buildUi.buildCmd->text());
    cg.writeEntry("Clean Command", buildUi.cleanCmd->text());
    cg.writeEntry("Quick Compile Command", buildUi.quickComp->text());
}


/******************************************************************/
void KateBuildView::slotNext()
{
    const int itemCount = buildUi.errTreeWidget->topLevelItemCount();
    if (itemCount == 0) {
        return;
    }

    QTreeWidgetItem *item = buildUi.errTreeWidget->currentItem();

    int i = (item == 0) ? -1 : buildUi.errTreeWidget->indexOfTopLevelItem(item);

    while (++i < itemCount) {
        item = buildUi.errTreeWidget->topLevelItem(i);
        if (!item->text(1).isEmpty()) {
            buildUi.errTreeWidget->setCurrentItem(item);
            slotItemSelected(item);
            return;
        }
    }
}

/******************************************************************/
void KateBuildView::slotPrev()
{
    const int itemCount = buildUi.errTreeWidget->topLevelItemCount();
    if (itemCount == 0) {
        return;
    }

    QTreeWidgetItem *item = buildUi.errTreeWidget->currentItem();

    int i = (item == 0) ? itemCount : buildUi.errTreeWidget->indexOfTopLevelItem(item);

    while (--i >= 0) {
        item = buildUi.errTreeWidget->topLevelItem(i);
        if (!item->text(1).isEmpty()) {
            buildUi.errTreeWidget->setCurrentItem(item);
            slotItemSelected(item);
            return;
        }
    }
}

/******************************************************************/
void KateBuildView::slotItemSelected(QTreeWidgetItem *item)
{
    // get stuff
    const QString filename = item->data(0, Qt::UserRole).toString();
    if (filename.isEmpty()) return;
    const int line = item->data(1, Qt::UserRole).toInt();
    const int column = item->data(2, Qt::UserRole).toInt();

    // open file (if needed, otherwise, this will activate only the right view...)
    m_win->openUrl(KUrl(filename));

    // any view active?
    if (!m_win->activeView()) {
        return;
    }

    // do it ;)
    m_win->activeView()->setCursorPosition(KTextEditor::Cursor(line-1, column));
    m_win->activeView()->setFocus();
}


/******************************************************************/
void KateBuildView::addError(const QString &filename, const QString &line,
                             const QString &column, const QString &message)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(buildUi.errTreeWidget);
    item->setBackground(1, Qt::gray);
    // The strings are twice in case kate is translated but not make.
    if (message.contains("error") ||
        message.contains(i18nc("The same word as 'make' uses to mark an error.","error")) ||
        message.contains("undefined reference") ||
        message.contains(i18nc("The same word as 'ld' uses to mark an ...","undefined reference"))
       )
    {
        item->setForeground(1, Qt::red);
        m_numErrors++;
    }
    if (message.contains("warning") ||
        message.contains(i18nc("The same word as 'make' uses to mark a warning.","warning"))
       )
    {
        item->setForeground(1, Qt::yellow);
        m_numWarnings++;
    }
    item->setTextAlignment(1, Qt::AlignRight);

    // visible text
    //remove path from visible file name
    KUrl file(filename);
    item->setText(0, file.fileName());
    item->setText(1, line);
    item->setText(2, message.trimmed());

    // used to read from when activating an item
    item->setData(0, Qt::UserRole, filename);
    item->setData(1, Qt::UserRole, line);
    item->setData(2, Qt::UserRole, column);

    // add tooltips in all columns
    item->setData(0, Qt::ToolTipRole, filename);
    item->setData(1, Qt::ToolTipRole, message);
    item->setData(2, Qt::ToolTipRole, message);
}

/******************************************************************/
KUrl KateBuildView::docUrl()
{
    KTextEditor::View *kv = mainWindow()->activeView();
    if (!kv) {
        kDebug() << "no KTextEditor::View" << endl;
        return KUrl();
    }
    
    if (kv->document()->isModified()) kv->document()->save();
    return kv->document()->url();
}

/******************************************************************/
bool KateBuildView::checkLocal(const KUrl &dir)
{
    if (dir.path().isEmpty()) {
        KMessageBox::sorry(0, i18n("There is no file or directory specified for building."));
        return false;
    }
    else if (!dir.isLocalFile()) {
        KMessageBox::sorry(0,  i18n("The file \"%1\" is not a local file. "
        "Non-local files cannot be compiled.", dir.path()));
        return false;
    }
    return true;
}

/******************************************************************/
bool KateBuildView::slotMake(void)
{
    KUrl dir(docUrl()); // docUrl() saves the current document
    
    if (buildUi.buildDir->text().isEmpty()) {
        if (!checkLocal(dir)) return false;
        // dir is a file -> remove the file with upUrl().
        dir = dir.upUrl();
    }
    else {
        dir = KUrl(buildUi.buildDir->text());
    }
    return startProcess(dir, buildUi.buildCmd->text());
}

/******************************************************************/
bool KateBuildView::slotMakeClean(void)
{
    KUrl dir(docUrl()); // docUrl() saves the current document
    
    if (buildUi.buildDir->text().isEmpty()) {
        if (!checkLocal(dir)) return false;
        // dir is a file -> remove the file with upUrl().
        dir = dir.upUrl();
    }
    else {
        dir = KUrl(buildUi.buildDir->text());
    }

    return startProcess(dir, buildUi.cleanCmd->text());
}

/******************************************************************/
bool KateBuildView::slotQuickCompile()
{
    QString cmd = buildUi.quickComp->text();
    if (cmd.isEmpty()) {
        KMessageBox::sorry(0, i18n("The custom command is empty."));
        return false;
    }
    
    KUrl url(docUrl());
    KUrl dir = url.upUrl();// url is a file -> remove the file with upUrl()
    // Check if the command contains the file name or directory
    if (cmd.contains("%f") || cmd.contains("%d")) {
        if (!checkLocal(url)) return false;
             
        cmd.replace("%f", url.toLocalFile());
        cmd.replace("%d", dir.toLocalFile());
    }
    
    return startProcess(dir, cmd);
}

/******************************************************************/
bool KateBuildView::startProcess(const KUrl &dir, const QString &command)
{
    if (m_proc->state() != QProcess::NotRunning) {
        return false;
    }

    // clear previous runs
    buildUi.plainTextEdit->clear();
    buildUi.errTreeWidget->clear();
    m_output_lines.clear();
    m_numErrors = 0;
    m_numWarnings = 0;
    m_make_dir_stack.clear();
    
    // activate the output tab
    buildUi.ktabwidget->setCurrentIndex(1);

    // set working directory
    m_make_dir = dir;
    m_make_dir_stack.push(m_make_dir);
    m_proc->setWorkingDirectory(m_make_dir.toLocalFile(KUrl::AddTrailingSlash));
    m_proc->setShellCommand(command);
    m_proc->setOutputChannelMode(KProcess::SeparateChannels);
    m_proc->start();

    if(!m_proc->waitForStarted(500)) {
        KMessageBox::error(0, i18n("Failed to run \"%1\". exitStatus = %2", command, m_proc->exitStatus()));
        return false;
    }

    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
    return true;
}


/******************************************************************/
bool KateBuildView::slotStop()
{
    if (m_proc->state() != QProcess::NotRunning) {
        m_proc->terminate();
        return true;
    }
    return false;
}

/******************************************************************/
void KateBuildView::slotProcExited(int exitCode, QProcess::ExitStatus)
{
    QApplication::restoreOverrideCursor();

    // did we get any errors?
    if (m_numErrors || m_numWarnings || (exitCode != 0)) {
        buildUi.ktabwidget->setCurrentIndex(0);
        buildUi.errTreeWidget->resizeColumnToContents(0);
        buildUi.errTreeWidget->resizeColumnToContents(1);
        //buildUi.errTreeWidget->setSortingEnabled(true);
        m_win->showToolView(m_toolView);
    }

    if (m_numErrors || m_numWarnings) {
        QStringList msgs;
        if (m_numErrors) {
            msgs << i18np("Found one error.", "Found %1 errors.", m_numErrors);
        }
        if (m_numWarnings) {
            msgs << i18np("Found one warning.", "Found %1 warnings.", m_numWarnings);
        }
        KPassivePopup::message(i18n("Make Results"), msgs.join("\n"), m_toolView);
    } else {
        KPassivePopup::message(i18n("Make Results"), i18n("Build completed without problems."), m_toolView);
    }

}


/******************************************************************/
void KateBuildView::slotReadReadyStdOut()
{
    // read data from procs stdout and add
    // the text to the end of the output
    // FIXME This works for utf8 but not for all charsets
    QString l= QString::fromUtf8(m_proc->readAllStandardOutput());
    l.remove('\r');
    m_output_lines += l;

    QString tmp;

    int end=0;


    // handle one line at a time
    do {
        end = m_output_lines.indexOf('\n');
        if (end < 0) break;
        end++;
        tmp = m_output_lines.mid(0, end);
        tmp.remove('\n');
        buildUi.plainTextEdit->appendPlainText(tmp);
        //kDebug() << tmp;
        if (tmp.indexOf(m_newDirDetector) >=0) {
            //kDebug() << "Enter/Exit dir found";
            int open = tmp.indexOf("`");
            int close = tmp.indexOf("'");
            KUrl newDir = KUrl(tmp.mid(open+1, close-open-1));
            kDebug () << "New dir = " << newDir;

            if ((m_make_dir_stack.size() > 1) && (m_make_dir_stack.top() == newDir)) {
                m_make_dir_stack.pop();
                newDir = m_make_dir_stack.top();
            }
            else {
                m_make_dir_stack.push(newDir);
            }

            m_make_dir = newDir;
        }


        m_output_lines.remove(0,end);

    } while (1);

}

/******************************************************************/
void KateBuildView::slotReadReadyStdErr()
{
    // FIXME This works for utf8 but not for all charsets
    QString l= QString::fromUtf8(m_proc->readAllStandardError());
    l.remove('\r');
    m_output_lines += l;

    QString tmp;

    int end=0;

    do {
        end = m_output_lines.indexOf('\n');
        if (end < 0) break;
        end++;
        tmp = m_output_lines.mid(0, end);
        tmp.remove('\n');
        buildUi.plainTextEdit->appendPlainText(tmp);

        processLine(tmp);

        m_output_lines.remove(0,end);

    } while (1);

}

/******************************************************************/
void KateBuildView::processLine(const QString &line)
{
    QString l = line;
    kDebug() << l ;

    //look for a filename
    if (l.indexOf(m_filenameDetector)<0)
    {
        addError(QString(), 0, QString(), l);
        //kDebug() << "A filename was not found in the line ";
        return;
    }

    int match_start = m_filenameDetector.indexIn(l, 0);
    int match_len = m_filenameDetector.matchedLength();

    QString file_n_line = l.mid(match_start, match_len);

    int name_end = file_n_line.lastIndexOf(':');
    QString filename = file_n_line.left(name_end);
    QString line_n = file_n_line.mid(name_end+1);
    QString msg = l.remove(m_filenameDetector);

    //kDebug() << "File Name:"<<filename<< " msg:"<< msg;
    //add path to file
    if (QFile::exists(m_make_dir.toLocalFile(KUrl::AddTrailingSlash)+filename)) {
        filename = m_make_dir.toLocalFile(KUrl::AddTrailingSlash)+filename;
    }

    // Now we have the data we need show the error/warning
    addError(filename, line_n, QString(), msg);

}


/******************************************************************/
void KateBuildView::slotBrowseClicked()
{
    KUrl defDir(buildUi.buildDir->text());

    if (buildUi.buildDir->text().isEmpty()) {
        // try current document dir
        KTextEditor::View *kv = mainWindow()->activeView();
        if (kv != 0) {
            defDir = kv->document()->url();
        }
    }

    buildUi.buildDir->setText(KFileDialog::getExistingDirectory(defDir, 0, QString()));
}



