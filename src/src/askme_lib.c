
#define _POSIX_C_SOURCE    200112L
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "askme_lib.h"

#include "ds_str.h"
#include "ds_array.h"

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

static void create_dir (const char *path, ...)
{
   char *fullpath = NULL;
   va_list ap;
   struct stat sb;
   static const int mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

   va_start (ap, path);
   if (!(fullpath = ds_str_vcat (path, ap))) {
      ASKME_LOG ("OOM Error, cannot create path [%s/...]\n", path);
      return;
   }
   if ((stat (fullpath, &sb))!=0) {
      ASKME_LOG ("[%s]: %m, attempting to create it.\n", fullpath);
      if ((mkdir (fullpath, mode))!=0) {
         ASKME_LOG ("[%s]: Cannot create: %m\n", fullpath);
      }
   }
   free (fullpath);
   va_end (ap);
}

static void create_homedir ()
{
   char *homedir = NULL;
#ifdef PLATFORM_WINDOWS
   homedir = ds_str_cat (getenv ("HOMEDRIVE"), getenv ("HOMEPATH"), "/.askme/", NULL);
#endif
#ifdef PLATFORM_POSIX
   homedir = ds_str_cat (getenv ("HOME"), "/.askme", NULL);
#endif
   create_dir (homedir, NULL);
   create_dir (homedir, "/topics", NULL);
   create_dir (homedir, "/grades", NULL);
   free (homedir);
}

char *askme_get_subdir (const char *path)
{
   char *homedir = NULL;

   create_homedir ();
#ifdef PLATFORM_WINDOWS
   homedir = ds_str_cat (getenv ("HOMEDRIVE"), getenv ("HOMEPATH"), "/.askme/", path, NULL);
#endif
#ifdef PLATFORM_POSIX
   homedir = ds_str_cat (getenv ("HOME"), "/.askme/", path, NULL);
#endif
   return homedir;
}

char **askme_list_topics (void)
{
   bool error = true;
   void **ret = NULL;
   DIR *dirp = NULL;
   struct dirent *de = NULL;
   char *tmp = NULL;

   char *topic_dir = askme_get_subdir ("topics");
   if (!topic_dir) {
      ASKME_LOG ("Failed to determine the topics directory path\n");
      goto errorexit;
   }

   if (!(dirp = opendir (topic_dir))) {
      ASKME_LOG ("Failed to open directory [%s] for reading\n", topic_dir);
      goto errorexit;
   }

   while ((de = readdir (dirp))) {
      if ((memcmp (de->d_name, ".", 2))==0 ||
          (memcmp (de->d_name, "..", 3))==0) {
         continue;
      }
      free (tmp);
      if (!(tmp = ds_str_dup (de->d_name))) {
         ASKME_LOG ("OOM error - cannot copy [%s]\n", de->d_name);
         goto errorexit;
      }
      if (!(ds_array_ins_tail (&ret, tmp))) {
         ASKME_LOG ("OOM error - failed to insert [%s] into array\n", tmp);
         goto errorexit;
      }
   }

   error = false;

errorexit:
   if (dirp) {
      closedir (dirp);
   }
   free (tmp);
   if (error) {
      char **tmp = (char **)ret;
      for (size_t i=0; tmp && tmp[i]; i++) {
         free (tmp[i]);
      }
      free (ret);
   }

   return (char **)ret;
}


