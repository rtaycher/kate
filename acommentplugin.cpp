/****************************************************************************
 **                - Artistic Comment KTextEditor-Plugin -                 **
 ** - Copyright (C) 2010 by Jonathan Schmidt-Dominé <devel@the-user.org> - **
 **                                  ----                                  **
 **   - This program is free software: you can redistribute it and/or -    **
 **   - modify it under the terms of the GNU General Public License as -   **
 ** - published by the Free Software Foundation, either version 2 of the - **
 **          - License, or (at your option) any later version. -           **
 **  - This program is distributed in the hope that it will be useful, -   **
 **   - but WITHOUT ANY WARRANTY; without even the implied warranty of -   **
 ** - MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU -  **
 **              - General Public License for more details. -              **
 ** - You should have received a copy of the GNU General Public License -  **
 **   - along with this program. If not, see <http://gnu.org/licenses> -   **
 ***************************************************************************/

#include "acommentplugin.h"
#include "acommentview.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocale>
#include <KAction>
#include <KActionCollection>
#include <KActionMenu>
#include <KMenu>
#include <KConfigGroup>
#include <KComponentData>

#include <QMap>

#include "artisticcomment.h"

K_PLUGIN_FACTORY(ACommentPluginFactory, registerPlugin<ACommentPlugin>("ktexteditor_acomment");)
K_EXPORT_PLUGIN(ACommentPluginFactory("ktexteditor_acomment", "ktexteditor_plugins"))

ACommentPlugin::ACommentPlugin(QObject *parent, const QVariantList &args)
        : KTextEditor::Plugin(parent), m_config(KSharedConfig::openConfig(KComponentData("ktexteditor_acomment", "", KComponentData::SkipMainComponentRegistration), "", KConfig::CascadeConfig))
{
    Q_UNUSED(args);
    readConfig();
    instance = this;
}

ACommentPlugin::~ACommentPlugin()
{
    writeConfig();
}

void ACommentPlugin::addView(KTextEditor::View *view)
{
    ACommentView *nview = new ACommentView(view);
    m_views.append(nview);
}

void ACommentPlugin::removeView(KTextEditor::View *view)
{
    for (int z = 0; z < m_views.size(); z++)
    {
        if (m_views.at(z)->parentClient() == view)
        {
            ACommentView *nview = m_views.at(z);
            m_views.removeAll(nview);
            delete nview;
        }
    }
}

static ArtisticComment::type_t toTypeEnum(const QString& str)
{
    switch (str.size())
    {
    case 10:
        return ArtisticComment::LeftNoFill;
    case 6:
        return ArtisticComment::Center;
    case 5:
        return ArtisticComment::Right;
    default:
        return ArtisticComment::Left;
    }
}

void ACommentPlugin::readConfig()
{
    m_styles.clear();
    QStringList groups = m_config->groupList();
#define E(str) sub.readEntry(str)
#define ED(str, def) sub.readEntry(str, def)
    foreach(QString str, groups)
    {
        KConfigGroup sub = m_config->group(str);
        m_styles[str] = ArtisticComment(ED("begin", ""), ED("end", ""),
                                        ED("lineBegin", ""), ED("lineEnd", ""),
                                        ED("textBegin", ""), ED("textEnd", ""),
                                        ED("lfill", " ")[0], ED("rfill", " ")[0],
                                        ED("minfill", (size_t)0), ED("realWidth", (size_t)60), ED("truncate", true), toTypeEnum(E("type")));
        kDebug() << str << m_styles[str].begin << m_styles[str].realWidth << m_styles[str].lfill;
    }
//     m_styles["Nice C++ License"] = ArtisticComment("/***********************************************************", " ***********************************************************/", " ** ", "", "", " ", ' ', '*', 1, 60, false, ArtisticComment::Left);
//     m_styles["Doxygen/Javadoc"] = ArtisticComment("/**", " */", " * ", "", "", "", '\0', '\0', 0, 60, true, ArtisticComment::LeftNoFill);
#undef ED
#undef E
}

ACommentPlugin *ACommentPlugin::instance = 0;

void ACommentPlugin::writeConfig()
{
    kDebug() << "acomment";
    QStringList groups = m_styles.keys();
    #define E(str) sub->writeEntry(#str, ac.str);
    foreach(QString str, groups)
    {
        kDebug() << str;
        KConfigGroup *sub = new KConfigGroup(&*m_config, str);
        ArtisticComment& ac(m_styles[str]);
        E(begin)
        E(end)
        E(lineBegin)
        E(lineEnd)
        E(textBegin)
        E(textEnd)
        sub->writeEntry("lfill", QString(ac.lfill));
        sub->writeEntry("rfill", QString(ac.rfill));
        E(minfill)
        E(realWidth)
        E(truncate)
        switch (ac.type)
        {
        case ArtisticComment::LeftNoFill:
            sub->writeEntry("type", "LeftNoFill");
            break;
        case ArtisticComment::Left:
            sub->writeEntry("type", "Left");
            break;
        case ArtisticComment::Center:
            sub->writeEntry("type", "Center");
            break;
        case ArtisticComment::Right:
            sub->writeEntry("type", "Right");
            break;
        }
    }
#undef E
}

ACommentView::ACommentView(KTextEditor::View *view)
        : QObject(view)
        , KXMLGUIClient(view)
        , m_view(view)
{
    setComponentData(ACommentPluginFactory::componentData());

    KActionMenu *action = new KActionMenu(i18n("Insert \"Artistic Comment\""), this);
    KMenu *menu = action->menu();
    QStringList groups = ACommentPlugin::instance->m_styles.keys();
    foreach(QString entry, groups)
        menu->addAction(entry);
    actionCollection()->addAction("tools_acomment", action);
    //action->setShortcut(Qt::CTRL + Qt::Key_XYZ);
    connect(menu, SIGNAL(triggered(QAction*)), this, SLOT(insertAComment(QAction*)));

    setXMLFile("acommentui.rc");
}

ACommentView::~ACommentView()
{
}

void ACommentView::insertAComment(QAction *action)
{
    m_view->document()->replaceText(m_view->selectionRange(), ACommentPlugin::instance->m_styles[action->iconText()].apply(m_view->selectionText()));
}

#include "acommentview.moc"
