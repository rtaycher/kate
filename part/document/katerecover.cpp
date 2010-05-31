#include "katerecover.h"

MyWidget::MyWidget(QWidget *parent)
	: QWidget(parent)
	, option(NOTDEF)
{
	
	QPushButton *recover = new QPushButton("Recover");
	QPushButton *cancel = new QPushButton("Cancel");
	QLabel *label = new QLabel("A swap file exists on the disk. Do you want to recover the data?");
	QGridLayout *glayout = new QGridLayout;
	QHBoxLayout *hlayout = new QHBoxLayout;
	QVBoxLayout *vlayout = new QVBoxLayout;

	hlayout->addWidget(label);
	vlayout->addWidget(recover, Qt::AlignHCenter);
	vlayout->addWidget(cancel, Qt::AlignHCenter);

	glayout->addLayout(hlayout, 0, 0, Qt::AlignLeft);
	glayout->addLayout(vlayout, 0, 1, Qt::AlignRight);
	this->setLayout(glayout);

	//add signals connections
	connect(recover, SIGNAL(clicked()), this, SLOT(setOption(RECOVER)));
	connect(cancel, SIGNAL(clicked()), this, SLOT(setOption(CANCEL)));
	
}

char MyWidget::getOption()
{
	return this->option;
}

void MyWidget::setOption(char option)
{
	this->option = option;
}
