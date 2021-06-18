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
            if (j == REC_CCOUNT || j == REC_PCOUNT)
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

   if (last_asked[0] < last_asked[1])
      return 1;

   if (last_asked[0] > last_asked[1])
      return -1;

   return 0;
}

static double question_score (char **question)
{
   double pcount = 0.0, ccount = 1.0, score = 0.0, guessed = 0.0;
   sscanf (question[REC_PCOUNT], "%lf", &pcount);
   sscanf (question[REC_CCOUNT], "%lf", &ccount);

   // The smaller the pcount, the higher the probability that it was guessed
   // correctly. TODO: replace magic number with guess_factor.
   // for pcount = 1 : guess is 0.5
   //     pcount = 2 : guess is 0.4
   //     pcount = 3 : guess is 0.3
   static const double pcguesses[] = {
      0.1,
      0.2,
      0.4,
      0.75,
      0.9,
   };
   size_t pcidx = pcount + 1;

   guessed = pcidx < sizeof pcguesses / sizeof pcguesses[0] ?
                  pcguesses[pcidx] : 1.0;

   // ASKME_LOG ("%lf, %lf, %lf\n", pcount, ccount, guessed);

   score = ccount / pcount;
   if (pcount < 1.0)
      return score != score ? 0.0 : score;

   return score * guessed;
}

char **askme_question (char ***database, uint32_t flags)
{
   if (!database)
      return NULL;

   size_t nrecs = db_nrecs (database);
   if (nrecs <= 1)
      return database[0];

   // 1. Sort the database, presentation_time descending (newest first)
   qsort (database, nrecs, sizeof *database, record_compare);

   double lowest_time = 0.0;
   sscanf (database[0][REC_PTIME], "%lf", &lowest_time);

   // 2. For each threshold, find the first question with a score lower
   // than the threshold. Ask that question or the one immediately after.
   static const double thresholds[] = {
      0.5, 0.76, 0.81, 0.85, 0.88, 0.90, 0.91,
   };

   for (size_t t=0; t < sizeof thresholds / sizeof thresholds[0]; t++) {
      for (size_t j=0; database[j]; j++) {
         double score = question_score (database[j]);
         // ASKME_LOG ("[%s] %lf (%lf)\n", database[j][0], score, thresholds[t]);
         if (score < thresholds[t]) {
            return database[j];
         }
      }
   }

   // 3. If all the questions are above the highest threshold, return NULL so
   //    that caller knows the material has been mastered
   return NULL;
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



