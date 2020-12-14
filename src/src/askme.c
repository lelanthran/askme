#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "askme_lib.h"

#include "ds_str.h"

void randomise_array (void **array, size_t nitems)
{
   static int seed = 0;

   srand (seed);

   if (!seed) {
      // seed = time (NULL);
      // srand (seed);
   }

   for (size_t i=0; array[i]; i++) {
      size_t target = rand () % nitems;
      void *tmp = array[i];
      array[i] = array[target];
      array[target] = tmp;
   }
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
         ASKME_LOG ("Failed to find any topics. Maybe you should create some topic files\n"
                    "in the directory '$(HOME)/.askme/topics'.\n");
         goto errorexit;
      }

      printf ("Choose a topic from below (type in the number of the topic)\n");
      for (size_t i=0; topics && topics[i]; i++) {
         printf ("%zu: %s\n", i+1, topics[i]);
         ntopics++;
      }
      printf ("%s: ", prompt);

      size_t topic_number;
      static char input[1024];
      fgets (input, sizeof input, stdin);
      if ((sscanf (input, "%zu", &topic_number))!=1) {
         ASKME_LOG ("Failed to read a topic number, aborting\n");
         goto errorexit;
      }
      if (!topic_number || topic_number > ntopics) {
         ASKME_LOG ("Topic [%zu] does not exist\n", topic_number);
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
      ASKME_LOG ("Failed to load questions from [%s]\n", topic);
      goto errorexit;
   }

   size_t total_questions = 0;
   for (size_t i=0; questions[i]; i++) {
      total_questions++;
   }

   if (nquestions > total_questions) {
      nquestions = total_questions;
   }

   // Randomise the array
   randomise_array ((void **)questions, total_questions);


   // Generate the array to store the user responses
   if (!(responses = calloc (total_questions, sizeof *responses))) {
      ASKME_LOG ("OOM error: Failed to allocate response array of %zu elements\n",
                 total_questions);
      goto errorexit;
   }
   memset (responses, 0xff, sizeof responses);


   // Print the questions and store the responses
   for (size_t i=0; i<nquestions; i++) {
      printf ("Q-%05zu) %s\n", i+1, questions[i][ASKME_QIDX_QUESTION]);
      for (size_t j=2; questions[j]; j++) {
         printf ("   %zu: %s\n", j-2, questions[i][j]);
      }
      printf ("%s", prompt);
      fflush (stdout);
   }

   // Testing that we have all the results
   for (size_t i=0; questions[i]; i++) {
      printf ("%zu: ", i);
      for (size_t j=0; questions[i][j]; j++) {
         printf ("[%s] ", questions[i][j]);
      }
      printf ("\n");
   }

   ret = EXIT_SUCCESS;

errorexit:

   free (responses);

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

