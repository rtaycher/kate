#include "editinterface.h"
#include "editdcopinterface.h"
using namespace KTextEditor;

namespace KTextEditor
{
  class PrivateEditInterface
  {
  public:
    PrivateEditInterface()
    {
    interface = 0;
    }
    ~PrivateEditInterface(){}
  // Data Members
  EditDCOPInterface *interface;
  };

};

unsigned int EditInterface::globalEditInterfaceNumber = 0;

EditInterface::EditInterface()
{
  d = new PrivateEditInterface();
  globalEditInterfaceNumber++;
  myEditInterfaceNumber = globalEditInterfaceNumber;
  QString name = "EditInterface#" + QString::number(myEditInterfaceNumber);
  d->interface = new EditDCOPInterface(this, name.latin1());
}

EditInterface::~EditInterface()
{
  delete d->interface;
  delete d;
}

unsigned int EditInterface::editInterfaceNumber () const
{
  return myEditInterfaceNumber;
}
