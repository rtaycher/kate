#include "../factory/katefactory.h"

extern "C"
{
  void *init_libkwritepart()
  {
    return new KateFactory();
  }
}
