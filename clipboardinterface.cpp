#include "clipboardinterface.h"
#include "clipboarddcopinterface.h"
using namespace KTextEditor;

namespace KTextEditor
{
	class PrivateClipboardInterface
	{
	public:
		PrivateClipboardInterface()
		{
		interface = 0;
		}
		~PrivateClipboardInterface(){}
	// Data Members
	ClipboardDCOPInterface *interface;
	};

};

unsigned int ClipboardInterface::globalClipboardInterfaceNumber = 0;

ClipboardInterface::ClipboardInterface()
{
	d = new PrivateClipboardInterface();
	globalClipboardInterfaceNumber++;
        myClipboardInterfaceNumber = globalClipboardInterfaceNumber++;
	QString name = "ClipboardInterface#" + QString::number(myClipboardInterfaceNumber);
	d->interface = new ClipboardDCOPInterface(this, name.latin1());
}
ClipboardInterface::~ClipboardInterface()
{
  delete d->interface;
  delete d;
}

unsigned int ClipboardInterface::clipboardInterfaceNumber () const
{
  return myClipboardInterfaceNumber;
}
