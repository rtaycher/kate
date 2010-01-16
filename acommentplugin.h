#ifndef ACOMMENTPLUGIN_H
#define ACOMMENTPLUGIN_H

#include <KTextEditor/Plugin>

namespace KTextEditor
{
	class View;
}

#include "artisticcomment.h"

#include <QMap>

class ACommentView;

class ACommentPlugin
  : public KTextEditor::Plugin
{
  public:
    // Constructor
    explicit ACommentPlugin(QObject *parent = 0, const QVariantList &args = QVariantList());
    // Destructor
    virtual ~ACommentPlugin();

    void addView (KTextEditor::View *view);
    void removeView (KTextEditor::View *view);
 
    void readConfig();
    void writeConfig();
 
//     void readConfig (KConfig *);
//     void writeConfig (KConfig *);
 
  private:
    QMap<QString, ArtisticComment> m_styles;
    QList<class ACommentView*> m_views;
    static ACommentPlugin *instance;
    friend class ACommentView;
};

#endif
