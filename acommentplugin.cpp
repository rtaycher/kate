#include "acommentplugin.h"
#include "acommentview.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocale>
#include <KAction>
#include <KActionCollection>
#include <KConfigGroup>

#include <QMap>

#include "artisticcomment.h"

K_PLUGIN_FACTORY(ACommentPluginFactory, registerPlugin<ACommentPlugin>("ktexteditor_acomment");)
K_EXPORT_PLUGIN(ACommentPluginFactory("ktexteditor_acomment", "ktexteditor_plugins"))

ACommentPlugin::ACommentPlugin(QObject *parent, const QVariantList &args)
: KTextEditor::Plugin(parent)
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
	for(int z = 0; z < m_views.size(); z++)
	{
		if(m_views.at(z)->parentClient() == view)
		{
			ACommentView *nview = m_views.at(z);
			m_views.removeAll(nview);
			delete nview;
		}
	}
}

static ArtisticComment::type_t toTypeEnum(const QString& str)
{
    switch(str.size())
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
    KConfigGroup cg(KGlobal::config(), "ArtisticComment Plugin");
    m_styles.clear();
    QStringList groups = cg.groupList();
    #define E(str) sub.readEntry(str)
    #define ED(str, def) sub.readEntry(str, def)
    foreach(QString str, groups)
    {
        KConfigGroup sub = cg.group(str);
        m_styles[str] = ArtisticComment(E("begin"), E("end"),
                                        E("lineBegin"), E("lineEnd"),
                                        E("textBegin"), E("textEnd"),
                                        E("lfill")[0], E("rfill")[0],
                                        ED("minfill", (size_t)0), ED("realWidth", (size_t)60), ED("truncate", true), toTypeEnum(E("type")));
    }
    m_styles["Nice C++ License"] = ArtisticComment("/***********************************************************", " ***********************************************************/", " ** ", "", "", " ", ' ', '*', 1, 60, false, ArtisticComment::Left);
    #undef ED
    #undef E
}

ACommentPlugin *ACommentPlugin::instance = 0;

void ACommentPlugin::writeConfig()
{
    KConfigGroup cg(KGlobal::config(), "ArtisticComment Plugin");
    QStringList groups = m_styles.keys();
    #define E(str) sub->writeEntry(#str, ac.str);
    foreach(QString str, groups)
    {
        KConfigGroup *sub = new KConfigGroup(&cg, str);
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
        switch(ac.type)
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
	
	KAction *action = new KAction(i18n("KTextEditor - AComment"), this);
	actionCollection()->addAction("tools_acomment", action);
	//action->setShortcut(Qt::CTRL + Qt::Key_XYZ);
	connect(action, SIGNAL(triggered()), this, SLOT(insertAComment()));
	
	setXMLFile("acommentui.rc");
}

ACommentView::~ACommentView()
{
}

void ACommentView::insertAComment()
{
	m_view->document()->insertText(m_view->cursorPosition(), ACommentPlugin::instance->m_styles["Nice C++ License"].apply(m_view->selectionText()));
}

#include "acommentview.moc"
