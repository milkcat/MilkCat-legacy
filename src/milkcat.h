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

  // NORMAL_PROCESSOR is the HMM-model segmenter and POS tagger with CRF-model for 
  // Out-Of-Vocabulary Word & Named-Entity recognition.
  NORMAL_PROCESSOR = 0,

  // CRF_SEGMENTER is a CRF-model chinese word segmenter using the character tagging algorithm
  // It is better for Out-Of-Vocabulary word but not good at the recall on In-Vocabulary words
  // Moreover, it is slower than HMM-model word-based segmenter.
  CRF_SEGMENTER = 1,

  // CRF_PROCESSOR is a CRF-model chinese word segmenter and Part-of-Speech Tagger. 
  // segmention using CRF_SEGMENTER, and Part-of-Speech Tagging also using CRF model.
  CRF_PROCESSOR = 2,

  // HMM_PROCESSOR is the HMM-model segmenter and POS tagger only
  HMM_PROCESSOR = 3
};

typedef enum {
  MC_CHINESE_WORD = 0,
  MC_ENGLISH_WORD = 1,
  MC_NUMBER = 2,
  MC_SYMBOL = 3,
  MC_PUNCTION = 4,
  MC_OTHER = 5
} MC_WORD_TYPE;

// Initialize the MilkCat Process Instance, 
// processor_type is the type of processor, such as NORMAL_PROCESSOR ...
// model_dir_path is the path of model files with the trailing slash
EXPORT_API milkcat_t *milkcat_init(int processor_type, const char *model_dir_path);

// Delete the MilkCat Process Instance and release its resources
EXPORT_API void milkcat_delete(milkcat_t *processor);

// Start to Process a text
EXPORT_API void milkcat_process(milkcat_t *processor, const char *text);

// goto the next word in the text, if end of the text reached return 0 else return 1
EXPORT_API int milkcat_next(milkcat_t *processor);

// Get a term from current position
EXPORT_API const char *milkcat_get_word(milkcat_t *processor);

// Get the Part-Of-Speech Tag from current position
EXPORT_API const char *milkcat_get_part_of_speech_tag(milkcat_t *processor);

// Get the type of word, such as a number, a chinese word or an english word ...
EXPORT_API MC_WORD_TYPE milkcat_get_word_type(milkcat_t *processor);

// Get the error message if an error occurred
EXPORT_API const char *milkcat_get_error_message();

#ifdef __cplusplus
}
#endif

// ~endif of ifndef MILKCAT_H
#endif