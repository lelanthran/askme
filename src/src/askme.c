#define _POSIX_C_SOURCE 200809L

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include "askme.h"

#define REC_TIME        (3)
#define REC_PCOUNT      (4)
#define REC_CCOUNT      (5)

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
   while (tmp && (tmp = strchr (tmp, delim))) {
      nfields++;
      tmp++;
   }

   char **ret = calloc (nfields + 1, sizeof *ret);
   if (!ret)
      return NULL;

   char *copy = strdup (input);
   if (!copy) {
      askme_question_del (ret);
      return NULL;
   }
   char *nl = strchr (copy, '\n');
   if (nl)
      *nl = 0;

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

bool askme_db_fillrecs (char ***database, size_t nrecs)
{
   if (!database)
      return false;

   for (size_t i=0; database[i]; i++) {
      char **tmprec = realloc (database[i], (sizeof *tmprec) * (nrecs + 1));
      if (!tmprec)
         return false;

      for (size_t j=0; j<nrecs; j++) {
         if (!tmprec[j]) {
            tmprec[j] = j == REC_CCOUNT ? strdup ("0") : strdup ("1");
            tmprec[j+1] = NULL;
         }
      }
      database[i] = tmprec;
   }
   return true;
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
      char *nl = strchr (line, '\n');
      if (nl)
         *nl = 0;

      char **record = askme_split_fields (line, '\t');
      if (!record)
         goto errorexit;

      nrecords++;
      char ***tmp = realloc (ret, sizeof *ret * (nrecords + 2));
      if (!tmp)
         goto errorexit;

      ret = tmp;
      ret[nrecords - 1] = record;
      ret[nrecords] = NULL;
   }

   if (!(askme_db_fillrecs (ret, 7)))
      goto errorexit;

   error = false;

errorexit:
   if (infile)
      fclose (infile);

   if (error) {
      askme_db_del (&ret);
   }

   return ret;
}

bool askme_db_save (char ***database, const char *qfile)
{
   if (!database || !qfile)
      return false;

   FILE *outfile = NULL;

   if (!(outfile = fopen (qfile, "wt")))
      return false;

   for (size_t i=0; database[i]; i++) {
      const char *delim = "";
      for (size_t j=0; database[i][j]; j++) {
         fprintf (outfile, "%s%s", delim, database[i][j]);
         delim = "\t";
      }
      fprintf (outfile, "\n");
   }

   fclose (outfile);

   return true;
}

void askme_db_del (char ****database)
{
   if (!database || !*database)
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

#if 0
static size_t rec_nfields (char **rec)
{
   if (!rec)
      return 0;

   size_t ret = 0;
   for (size_t i=0; rec[i]; i++) {
      ret++;
   }

   return ret;
}

#endif

#if 1
void dbdump (const char *label, char ***database)
{
   printf ("[%s]\n", label);
   if (!database)
      printf ("NULL Database\n");
   for (size_t i=0; database[i]; i++) {
      for (size_t j=0; database[i][j]; j++) {
         printf ("%s, ", database[i][j]);
      }
      printf ("\n");
   }
}
#endif


bool askme_db_add (char ****database, const char *question, const char *answer)
{
   char ***tmpdb = NULL;
   size_t nrecords = db_nrecs (*database);

   // dbdump ("Before addition", *database);

   if (!(tmpdb = realloc (*database, (sizeof *database) * (nrecords + 2))))
      return false;

   (*database) = tmpdb;

   (*database)[nrecords] = NULL;
   (*database)[nrecords + 1] = NULL;

   char **newrec = calloc (6, sizeof *newrec);
   if (!newrec)
      return false;

   newrec [0] = strdup (question);
   newrec [1] = strdup (answer);
   if (!newrec[0] || !newrec[1]) {
      return false;
   }

   newrec[2] = newrec[3] = newrec[4] = newrec[5] = NULL;

   (*database)[nrecords] = newrec;

   // dbdump ("After addition", *database);

   return true;
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

bool db_inc_field (char ***database, const char *question, size_t field)
{
   size_t index = (size_t)0;
   while ((index = find_question (database, index, question))!=(size_t)-1) {
      size_t num = 0;
      char sznum[27];

      sscanf (database[index][field], "%zu", &num);
      num++;
      snprintf (sznum, sizeof sznum, "%zu", num);

      char *tmp = database[index][field];

      if (!(database[index][field] = strdup (sznum))) {
         database[index][field] = tmp;
         return false;
      }
      index++;
      free (tmp);
   }

   return true;
}

bool askme_db_inc_presentation (char ***database, const char *question)
{
   return db_inc_field (database, question, REC_PCOUNT);
}

bool askme_db_inc_correct (char ***database, const char *question)
{
   return db_inc_field (database, question, REC_CCOUNT);
}

//////////////////////////////////////////////////////////////////////////////////////
// Locate a suitable question

static double question_correct_percentage (const char **question)
{
   double pcount = 1.0, ccount = 0.0;

   sscanf (question[REC_PCOUNT], "%lf", &pcount);
   sscanf (question[REC_CCOUNT], "%lf", &ccount);
   return ccount / pcount;
}

int record_compare (const void *rec_lhs, const void *rec_rhs)
{
   const char **record[] = { NULL, NULL };
   size_t pcount[2] = { 1, 1 },
          ccount[2] = { 1, 1 };
   size_t ratio[2] = { 0, 0 };

   uint64_t last_asked[2] = {0, 0};

   int rand_result = (rand () % 3) - 1;

   if (!rec_lhs && rec_rhs)
      return 1;

   if (rec_lhs && !rec_rhs)
      return -1;

   record[0] = *(const char ***)rec_lhs;
   record[1] = *(const char ***)rec_rhs;

   sscanf (record[0][REC_CCOUNT], "%zu", &ccount[0]);
   sscanf (record[1][REC_CCOUNT], "%zu", &ccount[1]);

   sscanf (record[0][REC_PCOUNT], "%zu", &pcount[0]);
   sscanf (record[1][REC_PCOUNT], "%zu", &pcount[1]);

   sscanf (record[0][REC_TIME], "%" PRIu64, &last_asked[0]);
   sscanf (record[1][REC_TIME], "%" PRIu64, &last_asked[1]);

   if (pcount[0] == pcount[1] && pcount[0] < 2) {
      // ASKME_LOG ("pcount early return[%s:%s]: %zu\n", record[0][0], record[1][0], pcount[0]);
      return rand_result;
   }
   if (ccount[0] == ccount[1] && ccount[0] < 2) {
      // ASKME_LOG ("ccount early return[%s:%s]: %zu\n", record[0][0], record[1][0], ccount[0]);
      return rand_result;
   }

   ratio[0] = round (100.-1 * question_correct_percentage (record[0]));
   ratio[1] = round (100.0 * question_correct_percentage (record[1]));

   // ASKME_LOG ("ratio  return: [%s:%s] [%zu:%zu]\n", record[0][0], record[1][0],
                                                    // ratio[0], ratio[1]);

   if (ratio[0] < ratio[1])
      return 1;

   if (ratio[0] > ratio[1])
      return -1;

   if (last_asked[0] < last_asked[1])
      return 1;

   if (last_asked[0] > last_asked[1])
      return -1;

   return 0;
}

char **askme_question (char ***database)
{
   if (!database)
      return NULL;

   size_t nrecs = db_nrecs (database);
   if (nrecs <= 1)
      return database[0];

   // 1. Sort the database according to presentation:correct ratio, then by
   //    last-asked date.
   qsort (database, nrecs, sizeof *database, record_compare);

   size_t first_percentile = nrecs / 10;
   float percentage = question_correct_percentage ((const char **)database[first_percentile]);

   // 2. If 10% or more of the questions have a correct-ratio of >60%, then choose the
   //    first question that has a <60% correct-ratio.
   ASKME_LOG ("Found percentage = %lf in %zu\n", percentage, first_percentile);

   if (percentage > 0.6) {
      size_t index = first_percentile;
      while (index < nrecs) {
         if ((question_correct_percentage ((const char **)database[index])) < 0.6) {
            return database[index];
         }
         index++;
      }
   }

   // 2. Return the question at the top of the file.
   return database[nrecs-1];
}

void askme_question_del (char **question)
{
   if (!question)
      return;

   for (size_t i=0; question[i]; i++) {
      free (question[i]);
   }

   free (question);
}



