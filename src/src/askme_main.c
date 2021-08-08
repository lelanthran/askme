
#define _POSIX_C_SOURCE 200108

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>


#include <unistd.h>
#include <strings.h>

#include "askme.h"


/* A function to execute a child program and feed the input line-by-line back
 * to the calling program. The function mst be called with a callback function
 * that will receive each line as it is read in, a program to execute and the
 * command line arguments for that program.
 *
 * There are two versions of this function: one that takes the command line
 * arguments as variadic parameters delimited with a NULL pointer and one that
 * takes an array of parameters with a length specified for the array.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define MAX_LINE_LEN    ((1024 * 1024) * 10)    // 10MB lines

static char **parse_cline (int argc, char **argv)
{
   (void)argc;

   for (size_t i=0; argv[i]; i++) {

      if ((memcmp (argv[i], "--", 2))!=0)
         continue;

      char *argname = &argv[i][2];
      char *argvalue = strchr (argname, '=');
      if (argvalue)
         *argvalue = 0;
      else
         argvalue = " ";

      setenv (argname, &argvalue[1], 1);
      ASKME_LOG ("Set [%s:%s]\n", argname, &argvalue[1]);
   }
   return NULL;
}

typedef int (run_inferior_func_t) (const char *, void *);

static int run_inferior (run_inferior_func_t *callback,
                         void                *argp,
                         char                *command)
{
   int ret = EXIT_FAILURE;

   char *line = NULL;

   FILE *inf = NULL;


   if (!(inf = popen (command, "r")))
      goto errorexit;

   if (!(line = calloc (MAX_LINE_LEN, 1)))
      goto errorexit;

   size_t index = 0;
   int c = 0;

   while (!feof (inf) && !ferror (inf)  && (c = fgetc (inf))!=EOF) {
      if (c=='\n' || index>=MAX_LINE_LEN) {
         line[index++] = c;
         line[index] = 0;
         if (callback && callback (line, argp) < 0)
            goto errorexit;
         index = 0;
      } else {
         line[index++] = c;
      }
   }

errorexit:
   if (inf)
      pclose (inf);
   free (line);
   return ret;
}

static int collect_response (const char *line, void *arg)
{
   if (!arg)
      return -1;

   char ***dst = arg;

   // askme_question_del (*dst);
   *dst = askme_split_fields (line, '|');

   return *dst ? 1 : 0;
}

static int ignore_response (const char *line, void *arg)
{
   (void)line;
   (void)arg;
   return 0;
}

typedef size_t seconds_t;

static volatile sig_atomic_t g_end = 0;
void sigh (int n)
{
   if (n==SIGINT)
      g_end = 1;
}

int main (int argc, char **argv) // for now, no parameters
{
   parse_cline (argc, argv);

   // 1. Set the config values (duration between questions, etc).
   seconds_t question_interval = 1;
   const char *datadir = ".";
   const char *topic = "t-one.csv";

   char ***database = NULL;
   char *qfile = NULL;
   char *question_command = NULL;
   char *correction_command = NULL;
   char **question = NULL;
   char **response = NULL;

   uint32_t flags = 0;

   if (getenv ("datadir")) {
      datadir = getenv ("datadir");
   }

   if (getenv ("topic")) {
      topic = getenv ("topic");
   }

   if (getenv ("interval")) {
      sscanf (getenv ("interval"), "%zu", &question_interval);
   }

   signal (SIGINT, sigh);

   if (!(askme_printf (&qfile, "%s/%s", datadir, topic))) {
      ASKME_LOG ("Out of memory error creating filename [%s/%s]\n", datadir, topic);
      return EXIT_FAILURE;
   }

   while (!g_end) {
      // 2. Load the database.
      askme_db_del (&database);
      if (!(database = askme_db_load (qfile))) {
         ASKME_LOG ("Failed to load [%s]: %m\n", qfile);
         break;
      }

      // 3. Choose a question (TODO: Maybe some parameters for this? Thresholds, etc)
      // askme_question_del (question);
      question = askme_question (database, flags);
      if (!question) {
         if (!(flags & ASKME_FLAG_FORCE)) {
            run_inferior (NULL, NULL,
                          "zenity --info --no-wrap --text='"
                          "Material is mastered. Restart program with --force to ask questions "
                          "anyway."
                          "'");
         } else {
            run_inferior (NULL, NULL,
                          "zenity --info --no-wrap --text='"
                          "Error getting a new question. DB might be corrupt."
                          "'");
         }
         break;
      }

      // 4. Present the question.
      free (question_command);
      if (!(askme_printf (&question_command,
                  "zenity --forms "
                  "       --add-entry='%s'"
                  "       --add-entry='Add a new question'"
                  "       --add-entry='Answer for new question'",
                  question[0]))) {
         ASKME_LOG ("Failed to construct question '%s'\n", question[0]);
         break;
      }
      askme_question_del (response);
      response = NULL;

      if (!(run_inferior (collect_response, &response, question_command))) {
         ASKME_LOG ("Failed to execute [%s]: %m\n", question_command);
         break;
      }

      size_t counter = question_interval;
      if (!response || !response[0]) {
         while (!g_end && counter--) {
            sleep (1);
         }
         continue;
      }

      //    4.a) Update the database (presentation-counter++)
      askme_db_inc_presentation (database, question[0]);

      //    4.b) If the answer is correct, update the database (correct-counter++)
      //    4.c) If the answer is incorrect, present the correct answer.
      if ((strcasecmp (response[0], question[1]))==0) {
         ASKME_LOG ("CORRECT!!! [%s:%s]\n", response[0], question[1]);
         askme_db_inc_correct (database, question[0]);
      } else {
         ASKME_LOG ("INCORRECT!!! [%s:%s]\n", response[0], question[1]);
         free (correction_command);
         correction_command = NULL;
         askme_printf (&correction_command, "zenity --width=500px --warning "
                                            "--text='Correct answer: [%s]'", question[1]);
         run_inferior (ignore_response, NULL, correction_command);
      }

      //    4.d) If a new question is given, add it to the database
      if (response[1] && response[2] && response[1][0] && response[2][0]) {
         askme_db_add (&database, response[1], response[2]);
      }

      // 5. Save the database to file.
      // dbdump ("Saving", database);
      if (!(askme_db_save (database, qfile))) {
         ASKME_LOG ("Failed to save [%s]; %m\n", qfile);
         break;
      }

      // 6. Sleep for duration.
      while (!g_end && counter--) {
         sleep (1);
      }
      // 7. Goto #2.
   }

   askme_db_del (&database);
   free (qfile);
   free (question_command);
   free (correction_command);
   askme_question_del (response);

   return EXIT_SUCCESS;
}
