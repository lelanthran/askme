#define _POSIX_C_SOURCE 200809L

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "askme.h"

static size_t askme_vprintf (char **dst, const char *fmt, va_list ap)
{
   size_t ret = 0;
   size_t tmprc = 0;
   char *tmp = NULL;
   va_list ac;

   *dst = NULL;

   va_copy (ac, ap);
   int rc = vsnprintf (*dst, ret, fmt, ac);
   va_end (ac);

   ret = rc + 1;

   if (!(tmp = realloc (*dst, ret))) {
      return 0;
   }

   *dst = tmp;
   rc = vsnprintf (*dst, ret, fmt, ap);
   tmprc = rc;
   if (tmprc >= ret) {
      free (*dst);
      *dst = NULL;
      return 0;
   }

   return ret;
}

size_t askme_printf (char **dst, const char *fmt, ...)
{
   va_list ap;

   va_start (ap, fmt);
   size_t ret = askme_vprintf (dst, fmt, ap);
   va_end (ap);

   return ret;
}

char **askme_split_fields (const char *input, char delim)
{
   size_t nfields = 1;
   const char *tmp = input;
   while (tmp && (tmp = strchr (tmp, delim)))
      nfields++;

   char **ret = calloc (nfields + 1, sizeof *ret);
   if (!ret)
      return NULL;

   char *copy = strdup (input);
   if (!copy) {
      askme_question_del (ret);
      return NULL;
   }

   char delim_list[2] = { delim, 0 };
   char *field = strtok (copy, delim_list);
   size_t i=0;

   while (field) {
      if (!(ret[i++] = strdup (field))) {
         askme_question_del (ret);
         free (copy);
         return NULL;
      }
      field = strtok (NULL, delim_list);
   }

   free (copy);

   return ret;
}


char ***askme_db_load (const char *fname)
{
#define MAX_LINE_LENGTH       (1024 * 8)
   bool error = true;
   FILE *infile = NULL;
   static char line [MAX_LINE_LENGTH];
   char ***ret = NULL;
   size_t nrecords = 0;

   if (!fname)
      return NULL;

   if (!(infile = fopen (fname, "rt"))) {
      return NULL;
   }

   while (!feof (infile) && !ferror (infile) && (fgets (line, sizeof line -1, infile))) {
      char **record = askme_split_fields (line, '\t');
      if (!record)
         goto errorexit;

      nrecords++;
      char ***tmp = realloc (ret, sizeof *ret * (nrecords + 1));
      if (!tmp)
         goto errorexit;

      ret = tmp;
      ret[nrecords] = record;
      ret[nrecords + 1] = NULL;
   }

   error = false;

errorexit:
   if (error) {
      askme_db_del (&ret);
   }

   return ret;
}

   ///////////////////////////////////////////////////////////////////////////////////////////////
   int askme_db_save (const char *qfile);

void askme_db_del (char ****database)
{
   if (database || !*database)
      return;

   char ***tmp = *database;

   for (size_t i=0; tmp[i]; i++) {
      for (size_t j=0; tmp[i][j]; j++) {
         free (tmp[i][j]);
      }
      free (tmp[i]);
   }
   free (tmp);
   *database = NULL;
}

static size_t db_nrecs (char ***database)
{
   size_t ret = 0;
   if (!database)
      return 0;

   for (size_t i=0; database[i]; i++) {
      ret++;
   }

   return ret;
}

int askme_db_add (char ***database, const char *question, const char *answer)
{
   char ***tmpdb = NULL;
   size_t nrecords = db_nrecs (database);

   if (!(tmpdb = realloc (database, (sizeof *database) * (nrecords + 1))))
      return -1;

   database = tmpdb;

   nrecords++;
   database[nrecords + 1] = NULL;

   char **newrec = calloc (6, sizeof *newrec);
   if (!newrec)
      return -1;

   newrec [0] = strdup (question);
   newrec [1] = strdup (answer);
   if (!newrec[0] || !newrec[1]) {
      return -1;
   }

   newrec[2] = newrec[3] = newrec[4] = newrec[5] = NULL;

   database[nrecords] = newrec;

   return 0;
}

static size_t find_question (char ***database, size_t from, const char *question)
{
   for (size_t i=from; database[i]; i++) {
      if ((strcmp (database[i][0], question))==0) {
         return i;
      }
   }

   return (size_t)-1;
}

int db_inc_field (char ***database, const char *question, size_t field)
{
   size_t index = (size_t)-1;
   while ((index = find_question (database, index, question))!=(size_t)-1) {
      size_t num = 0;
      char sznum[27];
      sscanf (database[index][field], "%zu", &num);
      num++;
      snprintf (sznum, sizeof sznum, "%zu", num);
      char *tmp = database[index][field];
      if (!(database[index][field] = strdup (sznum))) {
         database[index][field] = tmp;
         return -1;
      }
      free (tmp);
   }

   return 0;
}

int askme_db_inc_presentation (char ***database, const char *question)
{
   return db_inc_field (database, question, 4);
}

int askme_db_inc_correct (char ***database, const char *question)
{
   return db_inc_field (database, question, 5);
}

   ///////////////////////////////////////////////////////////////////////////////////////////////
   // Locate a suitable question
   char **askme_question (char ***database);

void askme_question_del (char **question)
{
   if (!question)
      return;

   for (size_t i=0; question[i]; i++) {
      free (question[i]);
   }

   free (question);
}



