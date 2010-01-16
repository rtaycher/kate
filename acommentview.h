#ifndef ACOMMENTVIEW_H
#define ACOMMENTVIEW_H

#include <QObject>
#include <KXMLGUIClient>

class ACommentView : public QObject, public KXMLGUIClient
{
	Q_OBJECT
	public:
		explicit ACommentView(KTextEditor::View *view = 0);
		~ACommentView();
	private slots:
		void insertAComment();
	private:
		KTextEditor::View *m_view;
};

#endif
