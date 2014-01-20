//
// milkcat.h --- Created at 2013-09-03
//
// The MIT License (MIT)
//
// Copyright (c) 2013 ling0322 <ling032x@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//



#ifndef MILKCAT_H
#define MILKCAT_H

#include <stdbool.h>

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
typedef struct milkcat_model_t milkcat_model_t;
typedef struct milkcat_cursor_t milkcat_cursor_t;

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

typedef struct {
  const char *word;
  const char *part_of_speech_tag;
  MC_WORD_TYPE word_type;
} milkcat_item_t;

EXPORT_API milkcat_t *milkcat_new(const char *model_path, int analyzer_type);

EXPORT_API milkcat_t *milkcat_new_with_model(milkcat_model_t *model, int analyzer_type);

// Delete the MilkCat Process Instance and release its resources
EXPORT_API void milkcat_destroy(milkcat_t *m);

// Start to Process a text
EXPORT_API milkcat_cursor_t *milkcat_analyze(milkcat_t *m, const char *text);

EXPORT_API void milkcat_cursor_release(milkcat_cursor_t *c);

// goto the next word in the text, if end of the text reached return 0 else return 1
EXPORT_API bool milkcat_cursor_get_next(milkcat_cursor_t *c, milkcat_item_t *next_item);

EXPORT_API milkcat_model_t *milkcat_model_new(const char *model_path);

EXPORT_API void milkcat_model_destory(milkcat_model_t *model);

EXPORT_API void milkcat_model_set_userdict(milkcat_model_t *model, const char *path);

// Get the error message if an error occurred
EXPORT_API const char *milkcat_last_error();

#ifdef __cplusplus
}
#endif

// ~endif of ifndef MILKCAT_H
#endif