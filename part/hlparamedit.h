#ifndef _HL_PARAM_EDIT_H_
#define _HL_PARAM_EDIT_H_

#include <qhbox.h>

class HLParamEdit:public QHBox
{
Q_OBJECT
public:
	HLParamEdit(QWidget *parent);
	~HLParamEdit();
	void ListParameter(QString listname);
	void TextParameter(int length, QString text,bool regExp=false);
	const QString &text();
private:
	class QLineEdit *textEdit;
	class QLabel *listLabel;
	class QPushButton *listChoose;
	class QPushButton *listNew;
	class QPushButton *listEdit;
/*private slots:
	void listEditClicked();
	void listNewClicked();
	void listChooseClicked();*/
signals:
	void textChanged(const QString&);
};
#endif
