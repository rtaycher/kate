#include "../factory/kantfactory.h"

extern "C"
{
  void *init_libkwritepart()
  {
    return new KantFactory();
  }
}
