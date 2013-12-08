/*
 * named_entity_recognitioin.cc
 * out_of_vocabulary_word_recognitioin.cc (2013-09-03)
 *
 * by ling0322 at 2013-08-27
 *
 */

#include <stdio.h>
#include <iostream>
#include <string.h>
#include "darts.h"
#include "utils.h"
#include "out_of_vocabulary_word_recognition.h"
#include "token_instance.h"
#include "crf_segmenter.h"

OutOfVocabularyWordRecognition *OutOfVocabularyWordRecognition::New(
    const CRFModel *crf_model, 
    const TrieTree *oov_property,
    Status &status) {

  OutOfVocabularyWordRecognition *self = new OutOfVocabularyWordRecognition();

  self->crf_segmenter_ = CRFSegmenter::New(crf_model, status);

  if (status.ok()) {
    self->term_instance_ = new TermInstance();
    self->oov_property_ = oov_property;
  }

  if (status.ok()) {
    return self;
  } else {
    delete self;
    return NULL;
  }
  
}

OutOfVocabularyWordRecognition::~OutOfVocabularyWordRecognition() {
  delete crf_segmenter_;
  crf_segmenter_ = NULL;

  delete term_instance_;
  term_instance_ = NULL;
}

OutOfVocabularyWordRecognition::OutOfVocabularyWordRecognition(): term_instance_(NULL),
                                                                  crf_segmenter_(NULL) {
}

void OutOfVocabularyWordRecognition::Process(TermInstance *term_instance,
                                             TermInstance *in_term_instance, 
                                             TokenInstance *in_token_instance) {
  int ner_begin_token = 0;
  int current_token = 0;
  int current_term = 0;
  int term_token_number;
  int current_token_type;
  int ner_term_number = 0;
  const char *term_str;
  const char *pos_tag;
  bool oov_flag = false,
       next_oov_flag = false;

  for (size_t i = 0; i < in_term_instance->size(); ++i) {
    term_token_number = in_term_instance->token_number_at(i);
    current_token_type = in_token_instance->token_type_at(current_token);
    term_str = in_term_instance->term_text_at(i);
    // printf("%d\n", ner_term_number);

    if (term_token_number > 1) {
      if (next_oov_flag == true) {
        oov_flag = true;
        next_oov_flag = false;
      } else {
        oov_flag = false;
      }
    } else if (current_token_type != TokenInstance::kChineseChar) {
      oov_flag = false;
      next_oov_flag = false;
    } else {
      int oov_property = oov_property_->Search(term_str);
      if (oov_property == kOOVBeginOfWord) {
        next_oov_flag = true;
        oov_flag = true;
      } else if (oov_property == kOOVFilteredWord) {
        oov_flag = false;
        next_oov_flag = false;
      } else {
        oov_flag = true;
        next_oov_flag = false;
      }
    }

    if (oov_flag) {
      ner_term_number++;

    } else {

      // Recognize token from ner_begin_token to current_token
      if (ner_term_number > 1) {
        RecognizeRange(in_token_instance, ner_begin_token, current_token);
        
        for (size_t j = 0; j < term_instance_->size(); ++j) {
          CopyTermValue(term_instance, current_term, term_instance_, j);
          current_term++;
        }
      } else if (ner_term_number == 1) {
        CopyTermValue(term_instance, current_term, in_term_instance, i - 1);
        current_term++;
      }

      CopyTermValue(term_instance, current_term, in_term_instance, i);
      ner_begin_token = current_token + term_token_number;
      current_term++;
      ner_term_number = 0;
    }

    current_token += term_token_number;
  } 
  
  // Recognize remained tokens
  if (ner_term_number > 1) {
    RecognizeRange(in_token_instance, ner_begin_token, current_token);
    for (size_t j = 0; j < term_instance_->size(); ++j) {
      CopyTermValue(term_instance, current_term, term_instance_, j);
      current_term++;
    }
    ner_begin_token = current_token + term_token_number;
  } else if (ner_term_number == 1) {
    CopyTermValue(term_instance, current_term, in_term_instance, in_term_instance->size() - 1);
    current_term++;
  }

  term_instance->set_size(current_term);
}

void OutOfVocabularyWordRecognition::CopyTermValue(TermInstance *dest_term_instance, 
                                                   int dest_postion, 
                                                   TermInstance *src_term_instance, 
                                                   int src_position)  {

  dest_term_instance->set_value_at(dest_postion,
                                   src_term_instance->term_text_at(src_position),
                                   src_term_instance->token_number_at(src_position),
                                   src_term_instance->term_type_at(src_position),
                                   src_term_instance->term_id_at(src_position));
}

void OutOfVocabularyWordRecognition::RecognizeRange(TokenInstance *token_instance, int begin, int end) {
  crf_segmenter_->SegmentRange(term_instance_, token_instance, begin, end);
}
