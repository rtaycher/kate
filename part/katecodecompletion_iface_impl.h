#ifndef __CODECOMPLETION_IMPL_H__
#define __CODECOMPLETION_IMPL_H__

#include <qvaluelist.h>
#include <qstringlist.h>
#include <qvbox.h>
#include <qlistbox.h>
#include <ktexteditor/codecompletioninterface.h>
//class KWrite;
class KDevArgHint;

class KateView;

class CodeCompletion_Impl : public QObject {
  Q_OBJECT
    public:

  CodeCompletion_Impl(KateView *view);

  void showArgHint ( QStringList functionList, const QString& strWrapping, const QString& strDelimiter );
  void showCompletionBox(QValueList<KTextEditor::CompletionEntry> complList,int offset=0);
  bool eventFilter( QObject *o, QEvent *e );

private:
  void updateBox(bool newCoordinate=false);
  KDevArgHint* m_pArgHint;
  KateView *m_view;
  QVBox *m_completionPopup;
  QListBox *m_completionListBox;
  QValueList<KTextEditor::CompletionEntry> m_complList;
  uint m_lineCursor;
  uint m_colCursor;
  int m_offset;

public slots:
	void slotCursorPosChanged();

signals:
    void completionAborted();
    void completionDone();
    void argHintHided();
};




#endif
