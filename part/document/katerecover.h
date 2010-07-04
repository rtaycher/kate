#ifndef _RECOVER_H
#define _RECOVER_H

#include <QApplication>
#include <QPushButton>
#include <QWidget>
#include <QLayout>
#include <QLabel>

namespace Kate {

const char recover = 0x01;
const char cancel = 0x01;
const char notdef = 0x02;

class MyWidget : public QWidget
{
	Q_OBJECT
	
	public:
		MyWidget(QWidget *parent = 0);
		
	private:
		char option;
		
	public:
		char getOption();
	
	Q_SIGNALS:
	  void recoverPressed ();
	  void cancelPressed ();
		
	protected Q_SLOTS:
	  void print();
//		void setOption(const char&);
};
}
#endif
