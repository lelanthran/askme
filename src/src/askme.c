#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

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

char **askme_split_response (const char *input)
{
   size_t nfields = 0;
   const char *tmp = input;
   while (tmp && (tmp = strchr (tmp, '|')))
      nfields++;

   char **ret = calloc (nfields + 1, sizeof *ret);
   if (!ret)
      return NULL;

   char *copy = strdup (input);
   if (!copy) {
      askme_question_del (ret);
      return NULL;
   }

   char *field = strtok (copy, "|");
   size_t i=0;

   while (field) {
      if (!(ret[i++] = strdup (field))) {
         askme_question_del (ret);
         free (copy);
         return NULL;
      }
      field = strtok (NULL, "|");
   }

   free (copy);

   return ret;
}


   char ***askme_db_load (const char *fname);
   int askme_db_save (const char *qfile);
   void askme_db_del (char ****questions);
   int askme_db_add (char ***database, const char *question, const char *answer);
   int askme_db_inc_presentation (char ***database, const char *question);
   int askme_db_inc_correct (char ***database, const char *question);

   // Locate a suitable question
   char **askme_question (char ***database);
   void askme_question_del (char **question);
