#include "../factory/katefactory.h"

extern "C"
{
  void *init_libkatepart()
  {
    return new KateFactory();
  }
}
