
#ifndef H_ASKME
#define H_ASKME

#include <stdio.h>

#define ASKME_LOG(...)           do {\
   fprintf (stderr, "%s:%i:", __FILE__, __LINE__);\
   fprintf (stderr, __VA_ARGS__);\
} while (0)


// Data is stored within files in $DATADIR. Each files name is a different topic. Each file is a
// collection of csv records. Each record has the following fields:
//    question
//    answer
//    creation-timestamp
//    last-asked-timestamp
//    presentation-counter
//    correct-counter

#ifdef __cplusplus
extern "C" {
#endif

   // Some utility functions
   size_t askme_printf (char **dst, const char *fmt, ...);
   char **askme_split_response (const char *input);

   // Load and free the questions
   char ***askme_db_load (const char *fname);
   int askme_db_save (const char *qfile);
   void askme_db_del (char ****questions);
   int askme_db_add (char ***database, const char *question, const char *answer);
   int askme_db_inc_presentation (char ***database, const char *question);
   int askme_db_inc_correct (char ***database, const char *question);

   // Locate a suitable question
   char **askme_question (char ***database);
   void askme_question_del (char **question);

#ifdef __cplusplus
};
#endif


#endif


