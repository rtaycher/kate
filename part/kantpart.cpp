#include "../factory/kantfactory.h"

extern "C"
{
  void *init_libkantpart()
  {
    return new KantFactory();
  }
}
