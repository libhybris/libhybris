#include <stddef.h>
#include <string.h>
#include <stdio.h>

int property_get(const char *key, char *value, const char *default_value)
{  
 char *ret = NULL; 
 printf("property_get: %s\n", key);
 /* default */
 {  
    ret = default_value;
 }  
 if (ret)
 {
  strcpy(value, ret);
  return strlen(ret);
 }
 else
 {
  return 0;
 }
}
int property_set(const char *key, const char *value)
{
   printf("property_set: %s %s\n", key, value);
}
