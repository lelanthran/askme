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

#define REC_PTIME       (3)
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

   char sznow[27];
   snprintf (sznow, sizeof sznow, "%" PRIu64, time (NULL));

   for (size_t i=0; database[i]; i++) {
      char **tmprec = realloc (database[i], (sizeof *tmprec) * (nrecs + 1));
      if (!tmprec)
         return false;

      for (size_t j=0; j<nrecs; j++) {
         if (!tmprec[j]) {
            const char *src = "1";
            if (j == REC_CCOUNT)
               src = "0";
            if (j == REC_PTIME)
               src = sznow;

            tmprec[j] = strdup (src);
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

static bool db_inc_field (char ***database, const char *question, size_t field)
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
   uint64_t now = time (NULL);
   char sznow[27];
   snprintf (sznow, sizeof sznow, "%" PRIu64, now);

   size_t index = (size_t)0;
   while ((index = find_question (database, index, question))!=(size_t)-1) {
      char *tmp = database[index][REC_PTIME];

      if (!(database[index][REC_PTIME] = strdup (sznow))) {
         database[index][REC_PTIME] = tmp;
         return false;
      }
      index++;
      free (tmp);
   }

   return db_inc_field (database, question, REC_PCOUNT);
}

bool askme_db_inc_correct (char ***database, const char *question)
{
   return db_inc_field (database, question, REC_CCOUNT);
}

//////////////////////////////////////////////////////////////////////////////////////
// Locate a suitable question

int record_compare (const void *rec_lhs, const void *rec_rhs)
{
   const char **record[] = { NULL, NULL };
   uint64_t last_asked[2] = {0, 0};

   if (!rec_lhs && rec_rhs)
      return 1;

   if (rec_lhs && !rec_rhs)
      return -1;

   record[0] = *(const char ***)rec_lhs;
   record[1] = *(const char ***)rec_rhs;

   sscanf (record[0][REC_PTIME], "%" PRIu64, &last_asked[0]);
   sscanf (record[1][REC_PTIME], "%" PRIu64, &last_asked[1]);

   if (last_asked[0] > last_asked[1])
      return 1;

   if (last_asked[0] < last_asked[1])
      return -1;

   return 0;
}

double score_question (char **question, double lowest_ptime)
{
   //
   // 1. The more incorrectly a question has been answered, the more likely it is
   //    to be displayed next.
   // 2. The more frequently a question has been presented, the less likely it is
   //    to be displayed next.
   // 3. The more recently a question has been presented, the less likely it is
   //    to be presented next.
   // 4. The records must be stored sorted by presentation time ascending. The
   //    ptime of any record is the ptime of the record minus the ptime of the
   //    first record.
   //
   // Higher = better chance of being presented.
   double pcount = 0.0, ccount = 1.0, ptime = 1.0, perc = 0.0;
   sscanf (question[REC_PCOUNT], "%lf", &pcount);
   sscanf (question[REC_CCOUNT], "%lf", &ccount);
   sscanf (question[REC_PTIME],  "%lf", &ptime);

   ptime -= lowest_ptime;

   perc = pcount/ccount;
   if (ccount == 0.0 && (rand () % 5))
      perc = 0.0;

   double score = (perc * 0.75)
                + ((1/pcount) * 0.2)
                + (ptime * 0.05);

   return score;
}

char **askme_question (char ***database)
{
   if (!database)
      return NULL;

   size_t nrecs = db_nrecs (database);
   if (nrecs <= 1)
      return database[0];

   // 1. Sort the database, presentation_time ascending
   qsort (database, nrecs, sizeof *database, record_compare);

   dbdump ("Sorted", database);

   double lowest_time = 0.0;
   sscanf (database[0][REC_PTIME], "%lf", &lowest_time);

   // 2. Update all the scores, keep track of the highest
   double highest_score = 0.0l;
   size_t highest_index = 0;
   for (size_t i=0; database[i]; i++) {
      double score = score_question (database[i], lowest_time);
      if (score > highest_score) {
         highest_score = score;
         highest_index = i;
      }
   }

   // Return the question with the highest score.
   return database[highest_index];
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



