
#ifndef H_ASKME_LIB
#define H_ASKME_LIB

#include <stdio.h>

#define ASKME_LOG(...)     do {\
   printf ("%s:%i:", __FILE__, __LINE__);\
   printf (__VA_ARGS__);\
} while (0)

#define ASKME_SETBIT(num,idx)          (num |= (1 << idx))
#define ASKME_CLRBIT(num,idx)          (num = num & ~(1 << idx))
#define ASKME_TSTBIT(num,idx)          (num & (1 << idx))


#define ASKME_QIDX_QUESTION      (0)
#define ASKME_QIDX_ANSBMP        (1)
#define ASKME_QIDX_OPTION_OFFS   (2)

#ifdef __cplusplus
extern "C" {
#endif

   void askme_read_cline (int argc, char **argv);
   char *askme_get_subdir (const char *path, ...);
   char **askme_list_topics (void);
   char ***askme_load_questions (const char *topic);
   char ***askme_parse_qfile (FILE *inf);
   void askme_randomise_questions (char ***questions);
   size_t askme_count_questions (char ***questions);


#ifdef __cplusplus
};
#endif

#endif

