/*
 * named_entity_recognitioin.h
 * out_of_vocabulary_word_recognitioin.h (2013-09-03)
 *
 * by ling0322 at 2013-08-26
 *
 */


#ifndef OUT_OF_VOCABULARY_WORD_RECOGNITION_H
#define OUT_OF_VOCABULARY_WORD_RECOGNITION_H

#include <stdio.h>
#include "darts.h"
#include "crf_segmenter.h"
#include "crf_pos_tagger.h"
#include "part_of_speech_tag_instance.h"
#include "token_instance.h"

class OutOfVocabularyWordRecognition {
 public:
  static OutOfVocabularyWordRecognition *Create(const char *crf_segment_model_path, 
                                                const char *ner_filter_word_path);
  ~OutOfVocabularyWordRecognition();
  void Process(TermInstance *term_instance,
               const TermInstance *in_term_instance, 
               const TokenInstance *in_token_instance);

  static const int kOOVBeginOfWord = 1;
  static const int kOOVFilteredWord = 2;

 private:
  TermInstance *term_instance_;
  CRFSegmenter *crf_segmenter_;
  Darts::DoubleArray *double_array_;

  OutOfVocabularyWordRecognition();

  void RecognizeRange(const TokenInstance *token_instance, int begin, int end);

  void CopyTermValue(TermInstance *dest_term_instance, 
                     int dest_postion, 
                     const TermInstance *src_term_instance, 
                     int src_position);
};


#endif