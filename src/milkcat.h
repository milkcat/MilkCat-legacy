//
// MilkCat: Yet Another Chinese Segmenter, Part-Of-Speech Tagger and Dependency Parser
//
// milkcat.h
// Created at 2013-09-02
//



#ifndef MILKCAT_H
#define MILKCAT_H

#ifdef _WIN32
#ifdef MILKCAT_EXPORTS
#define EXPORT_API __declspec(dllexport)
#else
#define EXPORT_API __declspec(dllimport) 
#endif
#else
#define EXPORT_API
#endif

typedef struct milkcat_t milkcat_t;

#ifdef __cplusplus
extern "C" {
#endif

enum {

  // DEFAULT_PROCESSOR is the Mixed Bigram & CRF segmenter and Mixed HMM & CRF
  // part-of-speech tagger
  DEFAULT_PROCESSOR = 0,

  // CRF_SEGMENTER is a CRF-model chinese word segmenter using the character tagging algorithm
  // It is better for Out-Of-Vocabulary word but not good at the recall on In-Vocabulary words
  // Moreover, it is slower than HMM-model word-based segmenter.
  CRF_SEGMENTER = 1,

  // CRF_PROCESSOR is a CRF-model chinese word segmenter and Part-of-Speech Tagger. 
  // segmention using CRF_SEGMENTER, and Part-of-Speech Tagging also using CRF model.
  CRF_PROCESSOR = 2,

  // DEFAULT_SEGMENTER is the Mixed Bigram & CRF segmenter 
  DEFAULT_SEGMENTER = 3,
};

typedef enum {
  MC_CHINESE_WORD = 0,
  MC_ENGLISH_WORD = 1,
  MC_NUMBER = 2,
  MC_SYMBOL = 3,
  MC_PUNCTION = 4,
  MC_OTHER = 5
} MC_WORD_TYPE;


EXPORT_API milkcat_t *milkcat_new(const char *model_path, int analyzer_type);

// Delete the MilkCat Process Instance and release its resources
EXPORT_API void milkcat_destroy(milkcat_t *m);

// Start to Process a text
EXPORT_API void milkcat_analyze(milkcat_t *m, const char *text);

// goto the next word in the text, if end of the text reached return 0 else return 1
EXPORT_API int milkcat_next_word(milkcat_t *m);

// Get a term from current position
EXPORT_API const char *milkcat_get_word(milkcat_t *m);

// Get the Part-Of-Speech Tag from current position
EXPORT_API const char *milkcat_get_postag(milkcat_t *m);

// Get the type of word, such as a number, a chinese word or an english word ...
EXPORT_API MC_WORD_TYPE milkcat_get_wordtype(milkcat_t *m);

// Get the error message if an error occurred
EXPORT_API const char *milkcat_last_error();

EXPORT_API void milkcat_set_userdict(milkcat_t *m, const char *path);

#ifdef __cplusplus
}
#endif

// ~endif of ifndef MILKCAT_H
#endif