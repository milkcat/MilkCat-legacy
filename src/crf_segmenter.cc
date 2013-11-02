/*
 * crf_segmenter.cc
 *
 * by ling0322 at 2013-08-17
 *
 */

#include <stdio.h>
#include <string>
#include <string.h>
#include "crf_segmenter.h"
#include "term_instance.h"
#include "token_instance.h"
#include "feature_extractor.h"
#include "utils.h"

class SegmentFeatureExtractor: public FeatureExtractor {
 public:
  void set_token_instance(const TokenInstance *token_instance) { token_instance_ = token_instance; }
  size_t size() const { return token_instance_->size(); }

  void ExtractFeatureAt(size_t position, char (*feature_list)[kFeatureLengthMax], int list_size) {
    assert(list_size == 1);
    if (token_instance_->token_type_at(position) == TokenInstance::kChineseChar) {
      strcpy(feature_list[0], token_instance_->token_text_at(position));
    } else {
      strcpy(feature_list[0], "。");
    }
  }

 private:
  const TokenInstance *token_instance_;
};

CRFSegmenter *CRFSegmenter::Create(const char *model_path) {
  char error_message[1024];
  CRFSegmenter *self = new CRFSegmenter();
  self->crf_model_ = CRFModel::Create(model_path);
  if (self->crf_model_ == NULL) {
    delete self;
    return NULL;
  }

  self->crf_tagger_ = new CRFTagger(self->crf_model_);
  self->feature_extractor_ = new SegmentFeatureExtractor();

  // Get the tag's value in CRF++ model
  self->S = self->crf_tagger_->GetTagId("S");
  self->B = self->crf_tagger_->GetTagId("B");
  self->B1 = self->crf_tagger_->GetTagId("B1");
  self->B2 = self->crf_tagger_->GetTagId("B2");
  self->M = self->crf_tagger_->GetTagId("M");
  self->E = self->crf_tagger_->GetTagId("E");

  if (self->S < 0 || self->B < 0 || self->B1 < 0 || self->B2 < 0 || 
      self->M < 0 || self->E < 0) {
    delete self;
    sprintf(error_message, "bad CRF++ segmenter model %s, unable to find S, B, B1, B2, M, E tag.", model_path);
    set_error_message(error_message);
    return NULL;
  }

  return self;
}

CRFSegmenter::~CRFSegmenter() {
  if (feature_extractor_ != NULL) {
    delete feature_extractor_;
    feature_extractor_ = NULL;
  }
  
  if (crf_tagger_ != NULL) {
    delete crf_tagger_;
    crf_tagger_ = NULL;
  }

  if (crf_model_ != NULL) {
    delete crf_model_;
    crf_model_ = NULL;
  }
}

CRFSegmenter::CRFSegmenter(): crf_tagger_(NULL), 
                              crf_model_(NULL),
                              feature_extractor_(NULL) {}

void CRFSegmenter::SegmentRange(TermInstance *term_instance, const TokenInstance *token_instance, int begin, int end) {
  std::string buffer;

  feature_extractor_->set_token_instance(token_instance);
  crf_tagger_->TagRange(feature_extractor_, begin, end);

  int tag_id;
  int term_count = 0;
  size_t i = 0;
  int token_count = 0;
  int term_type;
  for (i = 0; i < end - begin; ++i) {
    token_count++;
    buffer.append(token_instance->token_text_at(begin + i));

    tag_id = crf_tagger_->GetTagAt(i);
    // printf("%s\n", tag_set_->TagIdToTagString(tag_id));
    if (tag_id == S || tag_id == E) {

      if (tag_id == S) {
        term_type = token_type_to_word_type(token_instance->token_type_at(begin + i));
      } else {
        term_type = TermInstance::kChineseWord;
      }

      term_instance->set_value_at(term_count, buffer.c_str(), token_count, term_type);
      term_count++;
      token_count = 0;
      buffer.clear();
    }
  }

  if (!buffer.empty()) {
    term_instance->set_value_at(term_count, buffer.c_str(), token_count, TermInstance::kChineseWord);
    term_count++;
  }

  term_instance->set_size(term_count);
}