#include "kantpartfactory.h"

extern "C"
{
  void *init_libkwritepart()
  {
    return new KantPartFactory();
  }
}
