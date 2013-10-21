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

class OffsetTokenInstance;

class OutOfVocabularyWordRecognition {
 public:
  static OutOfVocabularyWordRecognition *Create(const char *crf_segment_model_path, 
                                                const char *crf_pos_tagger_model_path,
                                                const char *ner_filter_word_path);
  ~OutOfVocabularyWordRecognition();
  void Process(TermInstance *term_instance,
               PartOfSpeechTagInstance *part_of_speech_tag_instance, 
               const TermInstance *in_term_instance, 
               const PartOfSpeechTagInstance *in_part_of_speech_tag_instance, 
               const TokenInstance *in_token_instance);

 private:
  OffsetTokenInstance *offset_instance_;
  TermInstance *term_instance_;
  PartOfSpeechTagInstance *part_of_speech_tag_instance_;
  CRFSegmenter *crf_segmenter_;
  CRFPOSTagger *crf_pos_tagger_;
  Darts::DoubleArray *double_array_;

  OutOfVocabularyWordRecognition();

  void RecognizeRange(const TokenInstance *token_instance, int begin, int end);

  void CopyTermPOSTagValue(TermInstance *dest_term_instance, 
                           PartOfSpeechTagInstance *dest_part_of_speech_tag_instance,
                           int dest_postion, 
                           const TermInstance *src_term_instance, 
                           const PartOfSpeechTagInstance *src_part_of_speech_tag_instance,
                           int src_position);
  
  // Check if the chinese character is filtered in named entity recognition
  bool IsFiltered(const char *utf8_ch) {
    return double_array_->exactMatchSearch<int>(utf8_ch) >= 0;
  }
};


/**
 * This class is a new instance contains the range of [begin, end) 
 * from the original instance
 *
 */
class OffsetTokenInstance: public TokenInstance {
 public:
  OffsetTokenInstance(): TokenInstance() {
  }

  // Update it with a new TokenInstance with its begin and end
  void Update(const TokenInstance *original_instance, int begin, int end) {
    original_instance_ = original_instance;
    begin_ = begin;
    end_ = end;
    assert(begin < end && end <= original_instance->size());
  }

  // Get token string at position
  const char *token_text_at(int position) const { 
    assert(position < size());
    return original_instance_->token_text_at(position + begin_); 
  }

  // Get the token type at position
  int token_type_at(int position) const { 
    assert(position < size());
    return original_instance_->token_type_at(position + begin_); 
  }

  // Get the size of this instance
  int size() const { return end_ - begin_; }
   
 private:
  const TokenInstance *original_instance_;
  int begin_;
  int end_;

  DISALLOW_COPY_AND_ASSIGN(OffsetTokenInstance);
};

#endif