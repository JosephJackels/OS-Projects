#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  uint* p = (uint*)4095;
  uint* p2 = (uint*)4096;

  printf(1, "\nMaking allowed dereference at address: %d\n", p2);
  printf(1, "Result: %d\n\n", *p2);
  printf(1, "Making unallowed dereference at address: %d\n", p);
  printf(1, "Result %d\n\n", *p);
  exit();
}