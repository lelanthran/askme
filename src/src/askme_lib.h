
#ifndef H_ASKME_LIB
#define H_ASKME_LIB

#include <stdio.h>

#define ASKME_LOG(...)     do {\
   printf ("%s:%i:", __FILE__, __LINE__);\
   printf (__VA_ARGS__);\
} while (0)


#define ASKME_QIDX_NUM           (0)
#define ASKME_QIDX_QUESTION      (1)
#define ASKME_QIDX_ANSBMP        (2)
#define ASKME_QIDX_RESPONSE      (3)
#define ASKME_QIDX_OPTION_OFFS   (4)

#ifdef __cplusplus
extern "C" {
#endif

   void askme_read_cline (int argc, char **argv);
   char *askme_get_subdir (const char *path);
   char **askme_list_topics (void);
   char **askme_load_questions (const char *topic);


#ifdef __cplusplus
};
#endif

#endif

