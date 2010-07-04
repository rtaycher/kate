#include "katerecover.h"
#include "katetextbuffer.h"

namespace Kate {

MyWidget::MyWidget(QWidget *parent)
	: QWidget(parent)
	, option(notdef)
{
	
	QPushButton *recoverButton = new QPushButton("Recover");
	QPushButton *cancelButton = new QPushButton("Cancel");
	QLabel *label = new QLabel("A swap file exists on the disk. Do you want to recover the data?");
	QGridLayout *glayout = new QGridLayout;
	QHBoxLayout *hlayout = new QHBoxLayout;
	QVBoxLayout *vlayout = new QVBoxLayout;

	hlayout->addWidget(label);
	vlayout->addWidget(recoverButton, Qt::AlignHCenter);
	vlayout->addWidget(cancelButton, Qt::AlignHCenter);

	glayout->addLayout(hlayout, 0, 0, Qt::AlignLeft);
	glayout->addLayout(vlayout, 0, 1, Qt::AlignRight);
	
	if (parent && parent->layout())
	  delete parent->layout();
	
	if (parent)
	  parent->setLayout(glayout);
	else
  	this->setLayout(glayout);

	//add signals connections
	bool click = true;
	connect(recoverButton, SIGNAL(clicked()), this, SIGNAL(recoverPressed()));
	connect(cancelButton, SIGNAL(clicked()), this, SIGNAL(cancelPressed()));
	
	connect(recoverButton, SIGNAL(clicked()), this, SLOT(print()));
	
}

char MyWidget::getOption()
{
	return this->option;
}

void MyWidget::print()
{
  kDebug( 13020 ) << "+++++++++++++++++++++++++++===========================";
}

/*
void MyWidget::setOption(const char &option)
{
	this->option = option;
}*/
}
