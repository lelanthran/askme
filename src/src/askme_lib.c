
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

char *askme_get_subdir (const char *path, ...)
{
   char *ret = NULL;
   char *homedir = NULL;
   va_list ap;

   va_start (ap, path);

   create_homedir ();

#ifdef PLATFORM_WINDOWS
   homedir = ds_str_cat (getenv ("HOMEDRIVE"), getenv ("HOMEPATH"), "/.askme/", path, NULL);
#endif
#ifdef PLATFORM_POSIX
   homedir = ds_str_cat (getenv ("HOME"), "/.askme/", path, NULL);
#endif
   ret = ds_str_vcat (homedir, ap);
   free (homedir);
   va_end (ap);

   return ret;
}

char **askme_list_topics (void)
{
   bool error = true;
   void **array = NULL;
   char **ret = NULL;
   DIR *dirp = NULL;
   struct dirent *de = NULL;
   char *tmp = NULL;

   char *topic_dir = askme_get_subdir ("topics", NULL);
   if (!topic_dir) {
      ASKME_LOG ("Failed to determine the topics directory path\n");
      goto errorexit;
   }

   if (!(dirp = opendir (topic_dir))) {
      ASKME_LOG ("Failed to open directory [%s] for reading\n", topic_dir);
      goto errorexit;
   }

   if (!(array = ds_array_new ())) {
      ASKME_LOG ("Failed to allocate new array\n");
      goto errorexit;
   }

   while ((de = readdir (dirp))) {
      if ((memcmp (de->d_name, ".", 2))==0 ||
          (memcmp (de->d_name, "..", 3))==0) {
         continue;
      }

      if (!(tmp = ds_str_dup (de->d_name))) {
         ASKME_LOG ("OOM error - cannot copy [%s]\n", de->d_name);
         goto errorexit;
      }
      if (!(ds_array_ins_tail (&array, tmp))) {
         ASKME_LOG ("OOM error - failed to insert [%s] into array\n", tmp);
         goto errorexit;
      }
   }

   if (!(ret = calloc (ds_array_length (array) + 1, sizeof *ret))) {
      ASKME_LOG ("OOM error - failed to allocate return value\n");
      goto errorexit;
   }

   for (size_t i=0; i<ds_array_length (array); i++) {
      char *tmp1 = ds_array_index (array, i);
      if (!(ret[i] = ds_str_dup (tmp1))) {
         ASKME_LOG ("OOM error - failed to copy array element %zu (%s)\n", i, tmp1);
         goto errorexit;
      }
   }

   error = false;

errorexit:
   free (topic_dir);

   if (dirp) {
      closedir (dirp);
   }

   for (size_t i=0; i<ds_array_length (array); i++) {
      free (ds_array_index (array, i));
   }
   ds_array_del (array);

   if (error) {
      for (size_t i=0; ret && ret[i]; i++) {
         free (ret[i]);
      }
      free (ret);
      ret = NULL;
   }

   return ret;
}

char ***askme_load_questions (const char *topic)
{
   bool error = true;
   char ***ret = NULL;
   void **array = NULL;
   char *fullpath = NULL;
   FILE *inf = NULL;

   if (!(fullpath = askme_get_subdir ("topics/", topic, NULL))) {
      ASKME_LOG ("OOM error - unable to create pathname [topics/%s\n", topic);
      goto errorexit;
   }

   if (!(inf = fopen (fullpath, "rt"))) {
      ASKME_LOG ("Failed to open [%s]: %m\n", fullpath);
      goto errorexit;
   }

   error = false;

errorexit:
   if (inf)
      fclose (inf);

   free (fullpath);

   for (size_t i=0; i<ds_array_length (array); i++) {
      void **inner_array = ds_array_index (array, i);
      for (size_t j=0; j<ds_array_length (inner_array); j++) {
         char *tmp = ds_array_index (inner_array, j);
         free (tmp);
      }
      ds_array_del (inner_array);
   }
   ds_array_del (array);

   if (error) {
      for (size_t i=0; ret && ret[i]; i++) {
         for (size_t j=0; ret[i][j]; j++) {
            free (ret[i][j]);
         }
         free (ret[i]);
      }
      free (ret);
      ret = NULL;
   }

   return ret;
}

