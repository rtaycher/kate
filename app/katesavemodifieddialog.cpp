/* This file is part of the KDE project
   Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "katesavemodifieddialog.h"
#include "katesavemodifieddialog.moc"
#include <klocale.h>
#include <qlistview.h>
#include <klistview.h>
#include <kguiitem.h>
#include <kstdguiitem.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kencodingfiledialog.h>
#include <ktexteditor/encodinginterface.h>

class AbstractKateSaveModifiedDialogCheckListItem:public QCheckListItem {
public:
        AbstractKateSaveModifiedDialogCheckListItem(QListViewItem *parent,const QString& title, const QString& url):QCheckListItem(parent,title,QCheckListItem::CheckBox) {
                setText(1,url);
                setOn(true);
		setState(InitialState);
        }
        virtual ~AbstractKateSaveModifiedDialogCheckListItem() {
        }
        virtual bool synchronousSave(QWidget *dialogParent)=0;
	enum STATE{InitialState,SaveOKState,SaveFailedState};
	STATE state() const { return m_state;}
	void setState(enum STATE state) {
		m_state=state;
		KIconLoader *loader = KGlobal::instance()->iconLoader();
		switch (state) {
			case InitialState:
				setPixmap(0,QPixmap());
				break;
			case SaveOKState:
				setPixmap(0,loader->loadIcon("ok",KIcon::NoGroup,height()));
				break;
			case SaveFailedState:
				setPixmap(0,loader->loadIcon("cancel",KIcon::NoGroup,height()));
				break;
		}
	}
private:
	STATE m_state;
};

class KateSaveModifiedDocumentCheckListItem:public AbstractKateSaveModifiedDialogCheckListItem {
public:
	KateSaveModifiedDocumentCheckListItem(QListViewItem *parent,Kate::Document *document):AbstractKateSaveModifiedDialogCheckListItem(parent,document->docName(),document->url().prettyURL()){
		m_document=document;
	}
	virtual ~KateSaveModifiedDocumentCheckListItem() {
	}
	virtual bool synchronousSave(QWidget *dialogParent) {
		if (m_document->url().isEmpty() ) {
		        KEncodingFileDialog::Result r=KEncodingFileDialog::getSaveURLAndEncoding(
        	      	KTextEditor::encodingInterface(m_document)->encoding(),QString::null,QString::null,dialogParent,i18n("Save As (%1)").arg(m_document->docName()));
		        
			m_document->setEncoding( r.encoding );
			if (!r.URLs.isEmpty()) {
				KURL tmp = r.URLs.first();
        	  		if ( !m_document->saveAs( tmp ) ) {
					setState(SaveFailedState);
					setText(1,m_document->url().prettyURL());
					return false;
				} else {
					bool sc=m_document->waitSaveComplete();
					setText(1,m_document->url().prettyURL());
					if (!sc) {
						setState(SaveFailedState);
						return false;
					} else {
						setState(SaveOKState);
						return true;
					}
				}
			} else {
				setState(SaveFailedState);
				return false;
			}
		} else { //document has an exising location
          		if ( !m_document->save() ) {
				setState(SaveFailedState);
				setText(1,m_document->url().prettyURL());
				return false;
			} else {
				bool sc=m_document->waitSaveComplete();
				setText(1,m_document->url().prettyURL());
				if (!sc) {
					setState(SaveFailedState);
					return false;
				} else {
					setState(SaveOKState);
					return true;
				}
			}

		}
	
		return false;

	}
private:
	Kate::Document *m_document;
};

KateSaveModifiedDialog::KateSaveModifiedDialog(QWidget *parent, QPtrList<Kate::Document> documents):
	KDialogBase( parent, "KateSaveModifiedDialog", true, i18n("Closing documents ..."), Yes | No | Cancel) {

	KGuiItem yesItem=KStdGuiItem::yes();
	yesItem.setText(i18n("Save selected"));
	setButtonGuiItem(KDialogBase::Yes,yesItem);

	KGuiItem noItem=KStdGuiItem::dontSave();
	setButtonGuiItem(KDialogBase::No,noItem);

	KGuiItem cancelItem=KStdGuiItem::close();
	cancelItem.setText(i18n("Abort closing"));
	setButtonGuiItem(KDialogBase::Cancel,cancelItem);

	QVBox *box=makeVBoxMainWidget();
	new QLabel(i18n("<qt>Some of the objects  you intend to close contain modifications, which are not yet saved to disc. Please choose what you want do do about this</qt>"),box);
	m_list=new KListView(box);
	m_list->addColumn(i18n("Title"));
	m_list->addColumn(i18n("Location"));
	m_list->setRootIsDecorated(true);
	m_list->setResizeMode(QListView::LastColumn);
	if (0) {
		m_projectRoot=new QListViewItem(m_list,i18n("Projects"));
	} else m_projectRoot=0;
	if (documents.count()>0) {
		m_documentRoot=new QListViewItem(m_list,i18n("Documents"));
		const uint docCnt=documents.count();
		for (uint i=0;i<docCnt;i++) {
			new KateSaveModifiedDocumentCheckListItem(m_documentRoot,documents.at(i));
		}
		m_documentRoot->setOpen(true);
	} else m_documentRoot=0;
	connect(new QPushButton(i18n("Select All"),box),SIGNAL(clicked()),this,SLOT(slotSelectAll()));
}	

KateSaveModifiedDialog::~KateSaveModifiedDialog() {
}


static void selectItems(QListViewItem *root) {
	if (!root) return;
	for (QListViewItem *it=root->firstChild();it;it=it->nextSibling()) {
		((QCheckListItem*)it)->setOn(true);
	}
}

void KateSaveModifiedDialog::slotSelectAll() {
	selectItems(m_documentRoot);
}


void KateSaveModifiedDialog::slotUser2() {
	kdDebug()<<"KateSaveModifiedDialog::slotYes()"<<endl;
	if (doSave(m_documentRoot)) done(QDialog::Accepted);
}

void KateSaveModifiedDialog::slotUser1() {
	done(QDialog::Accepted);
}

bool KateSaveModifiedDialog::doSave(QListViewItem *root) {
	if (root) {
		for (QListViewItem *it=root->firstChild();it;it=it->nextSibling()) {
			AbstractKateSaveModifiedDialogCheckListItem *cit= (AbstractKateSaveModifiedDialogCheckListItem*)it;
			if (cit->isOn() && (cit->state()!=AbstractKateSaveModifiedDialogCheckListItem::SaveOKState)) {
				if (!cit->synchronousSave(this /*perhaps that should be the kate mainwindow*/)) {
					KMessageBox::sorry( this, i18n("Data you requested to be saved could not be written. Please choose how you want to proceed."));
					return false;
				}
			} else if ((!cit->isOn()) && (cit->state()==AbstractKateSaveModifiedDialogCheckListItem::SaveFailedState)) {
				cit->setState(AbstractKateSaveModifiedDialogCheckListItem::InitialState);
			}

		}
	}
	return true;
}

bool KateSaveModifiedDialog::queryClose(QWidget *parent,QPtrList<Kate::Document> documents) {
	KateSaveModifiedDialog d(parent,documents);
	return (d.exec()!=QDialog::Rejected);
}
