#include <kwritedialogs.h>
#include <kwritedialogs.moc>
#include <klocale.h>
#include <ktexteditor/editorchooser.h>
#include <qlayout.h>


KWriteEditorChooser::KWriteEditorChooser(QWidget *):
	KDialogBase(KDialogBase::Plain,i18n("Choose an Editor Component"),KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Cancel) 
{
	(new QVBoxLayout(plainPage()))->setAutoAdd(true);
	m_chooser=new KTextEditor::EditorChooser(plainPage(),"Editor Chooser");
	setMainWidget(m_chooser);
	m_chooser->readAppSetting();
}

KWriteEditorChooser::~KWriteEditorChooser() {
;
}

void KWriteEditorChooser::slotOk() {
	m_chooser->writeAppSetting();
	KDialogBase::slotOk();
}
