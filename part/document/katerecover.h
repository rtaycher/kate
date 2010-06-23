#ifndef _RECOVER_H
#define _RECOVER_H

#include <QApplication>
#include <QPushButton>
#include <QWidget>
#include <QLayout>
#include <QLabel>

static char recover = 0;
static char cancel = 1;
static char notdef = 2;

class MyWidget : public QWidget
{
	Q_OBJECT
	public:
		MyWidget(QWidget *parent = 0);
	private:
		char option;
	public:
		char getOption();
	protected Q_SLOTS:
		void setOption(char);
};

#endif
