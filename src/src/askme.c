#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <ctype.h>

#include "askme_lib.h"

#include "ds_str.h"


#define SYMBOL_TICK           "✓"
#define SYMBOL_CROSS          "✘"
#define SYMBOL_CIRCLE         "⬤"

#define COLOR_DEFAULT         "\x1b[0m"

#define COLOR_FG_BLACK        "\x1b[30m"
#define COLOR_FG_RED          "\x1b[31m"
#define COLOR_FG_GREEN        "\x1b[32m"
#define COLOR_FG_YELLOW       "\x1b[33m"
#define COLOR_FG_BLUE         "\x1b[34m"
#define COLOR_FG_CYAN         "\x1b[35m"
#define COLOR_FG_MAGENTA      "\x1b[36m"
#define COLOR_FG_WHITE        "\x1b[37m"

#define COLOR_BG_BLACK        "\x1b[40m"
#define COLOR_BG_RED          "\x1b[41m"
#define COLOR_BG_GREEN        "\x1b[42m"
#define COLOR_BG_YELLOW       "\x1b[43m"
#define COLOR_BG_BLUE         "\x1b[44m"
#define COLOR_BG_CYAN         "\x1b[45m"
#define COLOR_BG_MAGENTA      "\x1b[46m"
#define COLOR_BG_WHITE        "\x1b[47m"

size_t parse_numbers (const char *string)
{
   size_t ret = 0;
   const char *tmp = string;
   while (*tmp) {
      size_t number = 0;
      while (*tmp && !(isdigit (*tmp))) {
         tmp++;
      }
      if ((sscanf (tmp, "%zu", &number))!=1)
         continue;
      ASKME_SETBIT (ret, number);
      while (*tmp && isdigit (*tmp)) {
         tmp++;
      }
   }
   return ret;
}

char *printbin (size_t num, char *dst)
{
   char *ret = dst;
   for (int i=31; i>=0; i--) {
      *dst++ = (num >> i) & 0x01 ? '1' : '0';
      *dst = 0;
   }
   return ret;
}


static const char *help_msg[] = {
"askme: A program to help memorise study material",
"  --help            This message",
"  --num-questions   The number of questions to ask (default 10)",
"  --topic           The topic to be questioned on (default the first one",
"                    as sorted alphabetically).",
"  --show-grades     The grades for the selected topic will be displayed",
"                    and no test will be run.",
"",
"  Topics must be stored as a tab-seperated list of questions",
"in $HOME/.askme/topics. Each line comprises a single record",
"consisting of fields seperated by a tab character.",
"  The fields are in the following order:",
"     The multiple choice question",
"     A binary representation of the answer",
"     Option for answer A.",
"     Option for answer B.",
"     ...",
"     Option for answer n.",
"",
"  Askme will randomly choose 10 questions (or as many as the user",
"specifies with --num-questions) and present them to the user. Each",
"answer is recorded and at the end of the test all the questions asked",
"will be displayed with the correct answers and a final grade will be",
"given.",
"",
"  Grades are stored for each topic and the cumulative total will be",
"displayed at the end of the test. The test scores can also be seen",
"with the --show-grades option.",
};

static void print_msg (const char **msg)
{
   for (size_t i=0; msg[i]; i++) {
      printf ("%s\n", msg[i]);
   }
}

int main (int argc, char **argv)
{
   int ret = EXIT_FAILURE;

   size_t  nquestions = 10;
   char *topic = NULL;
   bool free_topic = false;
   const char *prompt = getenv ("PS2");
   char ***questions = NULL;
   uint64_t *responses = NULL;
   static char input[1024];

   char *out_option = NULL;
   const char *out_template = NULL;
   const char *out_response = NULL;

   if (!prompt || prompt[0] == 0) {
      prompt = "> ";
   }

   askme_read_cline (argc, argv);

   if (getenv ("help")) {
      print_msg (help_msg);
      ret = EXIT_SUCCESS;
      goto errorexit;
   }

   if (getenv ("num-questions")) {
      if ((sscanf (getenv ("num-questions"), "%zu", &nquestions))!=1) {
         ASKME_LOG ("Unable to read [%s] as a number\n", getenv ("num-questions"));
         goto errorexit;
      }
   }

   if (getenv ("show-grades")) {
      ASKME_LOG ("Unimplemented\n");
      goto errorexit;
   }

   free_topic = false;
   topic = getenv ("topic");

   if (!topic) {
      char **topics = askme_list_topics ();
      size_t ntopics = 0;
      if (!topics || !topics[0]) {
         ASKME_LOG (COLOR_FG_RED
                    "Failed to find any topics. Maybe you should create some topic files\n"
                    "in the directory '$(HOME)/.askme/topics'."
                    COLOR_DEFAULT "\n");
         goto errorexit;
      }

      printf (COLOR_BG_BLUE COLOR_FG_BLACK
              "Choose a topic from below (type in the number of the topic)"
              COLOR_DEFAULT "\n");

      for (size_t i=0; topics && topics[i]; i++) {
         printf ("%zu: %s\n", i+1, topics[i]);
         ntopics++;
      }
      printf ("%s: ", prompt);

      size_t topic_number;
      fgets (input, sizeof input, stdin);
      if ((sscanf (input, "%zu", &topic_number))!=1) {
         ASKME_LOG (COLOR_FG_RED "Failed to read a topic number, aborting" COLOR_DEFAULT "\n");
         goto errorexit;
      }
      if (!topic_number || topic_number > ntopics) {
         ASKME_LOG (COLOR_FG_RED "Topic [%zu] does not exist" COLOR_DEFAULT "\n", topic_number);
         goto errorexit;
      }
      free_topic = true;
      topic = ds_str_dup (topics[topic_number-1]);
      for (size_t i=0; topics[i]; i++) {
         free (topics[i]);
      }
      free (topics);
   }

   printf ("Seeking %zu questions from topic [%s]\n", nquestions, topic);
   if (!(questions = askme_load_questions (topic))) {
      ASKME_LOG (COLOR_FG_RED "Failed to load questions from [%s]" COLOR_DEFAULT "\n", topic);
      goto errorexit;
   }

   // Limit number of questions to what we actually have.
   size_t total_questions = askme_count_questions (questions);
   if (nquestions > total_questions) {
      nquestions = total_questions;
   }

   // Randomise the array
   askme_randomise_questions (questions);

   // Generate the array to store the user responses
   if (!(responses = calloc (total_questions, sizeof *responses))) {
      ASKME_LOG (COLOR_FG_RED "OOM error: Failed to allocate response array of %zu elements"
                 COLOR_DEFAULT "\n",
                 total_questions);
      goto errorexit;
   }
   memset (responses, 0xff, sizeof *responses);

   // Print the questions and store the responses
   for (size_t i=0; i<nquestions; i++) {
      bool answered = false;
      while (!answered && !feof (stdin) && !ferror (stdin)) {
         size_t noptions = 0;
         printf ("Q-%05zu) %s\n", i+1, questions[i][ASKME_QIDX_QUESTION]);
         for (size_t j=2; questions[i][j]; j++) {
            printf ("   %zu: %s\n", j-1, questions[i][j]);
            noptions++;
         }
         printf ("%s", prompt);
         fflush (stdout);
         fgets (input, sizeof input, stdin);

         char *tmp = strchr (input, '\n');
         if (tmp)
            *tmp = 0;
         tmp = input;

         if (tmp[0] == 'q' || tmp[0] == 'Q') {
            ASKME_LOG (COLOR_BG_RED COLOR_FG_BLACK "User requested exit." COLOR_DEFAULT "\n");
            i = nquestions;
            break;
         }

         bool not_number = false;
         for (size_t j=0; tmp[j]; j++) {
            if (!(isdigit (tmp[j])) && !(isspace (tmp[j]))) {
               ASKME_LOG (COLOR_FG_RED "Input at [%s] is not a valid number. Enter numbers separated by spaces"
                          COLOR_DEFAULT "\n",
                          &tmp[j]);
               not_number = true;
            }
         }
         if (not_number)
            continue;

         size_t response = parse_numbers (input);
         // char buf[65];
         // printbin (response, buf);
         // ASKME_LOG ("Response = [%s]\n", buf);
         bool too_large = false;
         for (size_t j=noptions+1; j<31; j++) {
            if (ASKME_TSTBIT (response, j)) {
               ASKME_LOG (COLOR_FG_RED "Response [%zu] is not an option" COLOR_DEFAULT "\n", j);
               too_large = true;
            }
         }
         if (too_large)
            continue;

         if (ASKME_TSTBIT (response, 0)) {
            ASKME_LOG (COLOR_FG_RED "Response [0] is not an option" COLOR_DEFAULT "\n");
            continue;
         }

         responses[i] = response;
         answered = true;
      }
   }

   size_t correct = 0;
   // First, print out all the correct answers
   for (size_t i=0; i<nquestions; i++) {
      size_t answer = askme_parse_answer (questions[i][ASKME_QIDX_ANSBMP]);
      if (answer == responses[i]) {
         printf ("Q-%05zu) %s: ", i+1, questions[i][ASKME_QIDX_QUESTION]);
         printf ("[" COLOR_FG_GREEN SYMBOL_TICK COLOR_DEFAULT "]\n");
         correct++;
      }
   }
   for (size_t i=0; i<nquestions; i++) {
      // Finally, print out all the wrong answers
      size_t answer = askme_parse_answer (questions[i][ASKME_QIDX_ANSBMP]);
      if (answer != responses[i]) {

         printf ("Q-%05zu) %s: ", i+1, questions[i][ASKME_QIDX_QUESTION]);
         printf ("[" COLOR_FG_RED SYMBOL_CROSS COLOR_DEFAULT "]\n");
         for (size_t j=ASKME_QIDX_OPTION_OFFS; questions[i][j]; j++) {
            size_t q_index = (j+1) - ASKME_QIDX_OPTION_OFFS;

            free (out_option); out_option = NULL;

            // Format the text of the option
            if (!(out_option = ds_str_cat ("   ", questions[i][j], NULL))) {
               ASKME_LOG ("OOM error\n");
               goto errorexit;
            }
            for (size_t i=strlen (out_option); i<40; i++) {
               if (!(ds_str_append (&out_option, "_", NULL))) {
                  ASKME_LOG ("OOM error\n");
                  goto errorexit;
               }
            }

            // Format the template
            out_template = "   ";
            if (ASKME_TSTBIT (answer, q_index)) {
               out_template = "[" COLOR_FG_BLUE SYMBOL_CIRCLE COLOR_DEFAULT "]";
            }

            // Format the response given
            out_response = "   ";
            if ((ASKME_TSTBIT (responses[i], q_index))) {
               out_response = ("[" COLOR_FG_GREEN SYMBOL_CIRCLE COLOR_DEFAULT "]");
            }
            fputs (out_option, stdout);   fputs (" ", stdout);
            fputs (out_template, stdout); fputs (" ", stdout);
            fputs (out_response, stdout); fputs (" ", stdout);
            fputs ("\n", stdout);
         }
      }
   }

   float perc = ((float)correct/nquestions) * 100;
   printf ("Final grade: %zu/%zu (%.0f%%)\n", correct, nquestions, perc);

   if (!(askme_save_grade (topic, correct, nquestions))) {
      ASKME_LOG ("Warning: Failed to save this grade: %m\n");
   }

   ret = EXIT_SUCCESS;

errorexit:

   free (responses);
   free (out_option);

   if (free_topic)
      free (topic);

   for (size_t i=0; questions && questions[i]; i++) {
      for (size_t j=0; questions[i][j]; j++) {
         free (questions[i][j]);
      }
      free (questions[i]);
   }
   free (questions);
   return ret;
}

