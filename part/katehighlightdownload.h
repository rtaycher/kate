#ifndef _KATEHIGHLIGHTDOWNLOAD_H_
#define _KATEHIGHLIGHTDOWNLOAD_H_

#include <kdialogbase.h>
#include <kio/jobclasses.h>
#include <qmemarray.h>

#define HLDOWNLOADPATH "http://kate.sourceforge.net/highlight/update.xml"

class HlDownloadDialog: public KDialogBase
{
	Q_OBJECT
public:
	HlDownloadDialog(QWidget *parent, const char *name, bool modal);
	~HlDownloadDialog();
private:
	class QListView	*list;
	class QString listData;
private slots:
	void listDataReceived(KIO::Job *, const QByteArray &data);

public slots:
	void slotUser1();

};

#endif

