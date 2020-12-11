
#ifndef H_ASKME_LIB
#define H_ASKME_LIB

#define ASKME_LOG(...)     do {\
   printf ("%s:%i:", __FILE__, __LINE__);\
   printf (__VA_ARGS__);\
} while (0)


#ifdef __cplusplus
extern "C" {
#endif

   void askme_read_cline (int argc, char **argv);



#ifdef __cplusplus
};
#endif

#endif

