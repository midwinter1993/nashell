#include "sys.h"

#include <stdio.h>
#include <unistd.h>
#include <limits.h>

const char* sys_curr_dir() {
   static char buf[PATH_MAX];
   if (getcwd(buf, sizeof(buf)) != NULL) {
       return buf;
   } else {
       fprintf(stderr, "Get current dir failure\n");
       return NULL;
   }
   return 0;
}
