#ifndef _KWRITEDIALOGS_H_
#define _KWRITEDIALOGS_H_

#include <kdialogbase.h>



namespace KTextEditor {
	class EditorChooser;
};

class KWriteEditorChooser: public KDialogBase {

Q_OBJECT

public:
	KWriteEditorChooser(QWidget *parent);
	virtual ~KWriteEditorChooser();
private:
	KTextEditor::EditorChooser *m_chooser;

protected slots:
	virtual void slotOk();
};


#endif
