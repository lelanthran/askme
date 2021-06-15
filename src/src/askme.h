
#ifndef H_ASKME
#define H_ASKME

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

   // Load and free the questions
   char ***askme_questions_load (const char *topic);
   void askme_questions_free (char ***questions);


#ifdef __cplusplus
};
#endif


#endif


