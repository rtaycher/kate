

  //  Radiate error messages back to kant


#include <kapp.h>
#include <kdebug.h>
#include <dcopclient.h>
#include "dcop_kant.h"


	int
ShowErrorMessage ( int argc, char** argv, QString strFileName, int nLine, QString strMessage  )
{

	KApplication app( argc, argv, "KantClient", false );
	kapp->dcopClient()->attach();
	QByteArray data;
	QCString strCommand ("ShowErrorMessage(QString, int, QString)");
	QCString strObject ("KantIface");
	QCString strApp ("kant");

	kdDebug() << "sending " << strCommand << " to object " << strObject << " in " << strApp << endl;
	QByteArray snd;
	QByteArray rcv;
	QCString _type_;
	QDataStream arg(snd, IO_WriteOnly);
	arg << strFileName << nLine << strMessage;

	if (kapp->dcopClient()->call( strApp, strObject, strCommand, snd, _type_, rcv ))
		kdDebug() << "okay" << endl;
	else
		kdDebug() << "nope" << endl;  //  TODO: Why not?

//	kdDebug() << _type_ << endl;
//	if( _type_ != "void" ) kdDebug() << "void expected, " << _type_.data() << " returned" << endl;

}

#ifdef SELF_TEST

	int
main ( int argc, char** argv )
{

	ShowErrorMessage ( argc, argv, "kantmainwindow.cpp", 42, 
				   "I dislike this line for some strange reason" );

}

#endif  //  SELF_TEST
