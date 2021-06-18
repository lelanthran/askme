
#ifndef H_ASKME
#define H_ASKME

#include <stdio.h>
#include <stdbool.h>

#define ASKME_LOG(...)           do {\
   fprintf (stderr, "%s:%i:", __FILE__, __LINE__);\
   fprintf (stderr, __VA_ARGS__);\
} while (0)

#define ASKME_FLAG_FORCE         (1 << 0)

// Data is stored within files in $DATADIR. Each files name is a different topic. Each file is a
// collection of csv records. Each record has the following fields:
// 0  question
// 1  answer
// 2  creation-timestamp
// 3  last-asked-timestamp
// 4  presentation-counter
// 5  correct-counter

#ifdef __cplusplus
extern "C" {
#endif

   // Some utility functions
   size_t askme_printf (char **dst, const char *fmt, ...);
   char **askme_split_fields (const char *input, char delim);
   void dbdump (const char *label, char ***database);

   // Load and free the questions
   char ***askme_db_load (const char *fname);
   bool askme_db_save (char ***database, const char *qfile);
   void askme_db_del (char ****database);
   bool askme_db_add (char ****database, const char *question, const char *answer);
   bool askme_db_inc_presentation (char ***database, const char *question);
   bool askme_db_inc_correct (char ***database, const char *question);

   // Locate a suitable question
   char **askme_question (char ***database, uint32_t flags);
   void askme_question_del (char **question);

#ifdef __cplusplus
};
#endif


#endif


