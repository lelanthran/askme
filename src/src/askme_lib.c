
#define _POSIX_C_SOURCE    200112L
#include <stdlib.h>
#include <string.h>

#include "askme_lib.h"

void askme_read_cline (int argc, char **argv)
{
   (void)argc;
   for (size_t i=0; argv[i]; i++) {
      if ((memcmp (argv[i], "--", 2))!=0)
         continue;
      char *name = &argv[i][2];
      char *value = strchr (name, '=');
      if (value) {
         *value++ = 0;
      } else {
         value = "";
      }
      setenv (name, value, 1);
   }
}
