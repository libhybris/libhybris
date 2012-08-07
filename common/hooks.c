
#include "properties.h"
#include <stdio.h>
#include <stddef.h>

void *get_hooked_symbol(char *sym)
{
  if (strcmp(sym, "property_get") == 0)
  {
    return property_get;
  }
  else if (strcmp(sym, "property_set") == 0)
  {
    return property_set;
  }
  else if (strcmp(sym, "printf") == 0)
  {
    return printf;
  }
  else return NULL;
}
