
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
   char *fullpath = NULL;
   char ***ret = NULL;
   FILE *inf = NULL;

   if (!(fullpath = askme_get_subdir ("topics/", topic, NULL))) {
      ASKME_LOG ("OOM error - unable to create pathname [topics/%s\n", topic);
      goto errorexit;
   }

   if (!(inf = fopen (fullpath, "rt"))) {
      ASKME_LOG ("Failed to open [%s]: %m\n", fullpath);
      goto errorexit;
   }

   if (!(ret = askme_parse_qfile (inf))) {
      ASKME_LOG ("Failed to parse [%s]: %m\n", fullpath);
      goto errorexit;
   }

   error = false;

errorexit:
   if (inf)
      fclose (inf);

   free (fullpath);

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

char ***askme_parse_qfile (FILE *inf)
{
   bool error = true;
   char ***ret = NULL;
   char *line = NULL;
   void **array = NULL;
   size_t line_len = 0;

   // TODO: Implement unit suffixes (MB, KB, etc)
   if (getenv ("line-length")) {
      if ((sscanf (getenv ("line-length"), "%zu", &line_len))!=1) {
         ASKME_LOG ("Failed to read [%s] as a line-length.\n", getenv ("line-length"));
         goto errorexit;
      }
   } else {
      line_len = 1024 * 1024 * 8; // 8MB ought to be enough as a default
   }

   if (!(array = ds_array_new ())) {
      ASKME_LOG ("OOM error: allocating new array\n");
      goto errorexit;
   }

   if (!(line = calloc (1, line_len))) {
      ASKME_LOG ("OOM error allocating line of [%zu] bytes\n", line_len);
      goto errorexit;
   }

   size_t recordnum = 0;
   while (!feof (inf) && !ferror (inf) && fgets (line, line_len - 1, inf)) {
      char *tmp = NULL;

      // Ensure that we have the entire line
      if (!(tmp = strchr (line, '\n'))) {
         ASKME_LOG ("Line %zu exceeds the maximum line length of %zu. Try "
                    "using the 'line-length' option to set a larger length"
                    "\non lines in the input file.", recordnum, line_len);
         goto errorexit;
      }
      *tmp = 0;
      char *saveptr = NULL;
      void **record_array = ds_array_new ();
      size_t fieldnum = 0;
      char *field = strtok_r (line, "\t", &saveptr);
      while (field != NULL) {
         char *newfield = ds_str_dup (field);
         if (!newfield) {
            ASKME_LOG ("OOM error - cannot allocate memory for record %zu, field %zu\n",
                       recordnum, fieldnum);
            goto errorexit;
         }
         if (!(ds_array_ins_tail (&record_array, newfield))) {
            ASKME_LOG ("Failed to add field %zu in record %zu\n", fieldnum, recordnum);
            free (newfield);
            goto errorexit;
         }
         fieldnum++;
         field = strtok_r (NULL, "\t", &saveptr);
      }
      if (!(ds_array_ins_tail (&array, record_array))) {
         ASKME_LOG ("Failed to insert record %zu into the results\n", recordnum);
         goto errorexit;
      }
      recordnum++;
   }

   if (!(ret = calloc (ds_array_length (array) + 1, sizeof *ret))) {
      ASKME_LOG ("OOM Error - failed to allocate return value\n");
      goto errorexit;
   }
   for (size_t i=0; i<ds_array_length (array); i++) {
      void **record = ds_array_index (array, i);
      if (!(ret[i] = calloc (ds_array_length (record) + 1, sizeof *ret[i]))) {
         ASKME_LOG ("Failed to allocate memory for record %zu\n", i);
         goto errorexit;
      }
      for (size_t j=0; j<ds_array_length (record); j++) {
         ret[i][j] = ds_array_index (record, j);
      }
   }

   error = false;

errorexit:

   free (line);

   for (size_t i=0; i<ds_array_length (array); i++) {
      void **inner_array = ds_array_index (array, i);
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

void askme_randomise_questions (char ***questions)
{
   static int seed = 0;

   srand (seed);

   if (!seed) {
      // seed = time (NULL);
      // srand (seed);
   }

   size_t nitems = askme_count_questions (questions);
   for (size_t i=0; questions[i]; i++) {
      size_t target = rand () % nitems;
      void *tmp = questions[i];
      questions[i] = questions[target];
      questions[target] = tmp;
   }
}

size_t askme_count_questions (char ***questions)
{
   size_t ret = 0;
   if (!questions)
      return 0;

   for (size_t i=0; questions[i]; i++) {
      ret++;
   }
   return ret;
}

size_t askme_parse_answer (const char *answer_string)
{
   size_t ret = 0;
   for (size_t i=0; answer_string && answer_string[i]; i++) {
      if (answer_string[i] == '1')     ret |= (1 << i);
      if (answer_string[i] == '0')     ret = ret;  // Do nothing
      if (answer_string[i]!='0' && answer_string[i]!='1') {
         ASKME_LOG ("Warning: answer template [%s] contains a '%c'. Only zeros and ones are allowed\n",
                    answer_string, answer_string[i]);
      }
   }
   return ret;
}
