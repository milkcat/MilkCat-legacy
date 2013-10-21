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
#include "segment_tag_set.h"
#include "term_instance.h"
#include "token_instance.h"
#include "feature_extractor.h"
#include "utils.h"

class SegmentFeatureExtractor: public FeatureExtractor {
 public:
  SegmentFeatureExtractor(): FeatureExtractor(1) {
  }

  void set_token_instance(const TokenInstance *token_instance) { token_instance_ = token_instance; }
  size_t size() const { return token_instance_->size(); }

  const char **ExtractFeatureAt(size_t position) {
    if (token_instance_->token_type_at(position) == TokenInstance::kChineseChar) {
      strcpy(feature_list_[0], token_instance_->token_text_at(position));
    } else {
      strcpy(feature_list_[0], "。");
    }

    return const_cast<const char **>(feature_list_);
  }

 private:
  const TokenInstance *token_instance_;
};

CRFSegmenter *CRFSegmenter::Create(const char *model_path) {
  CRFSegmenter *self = new CRFSegmenter();

  self->tag_set_ = new SegmentTagSet();
  self->tag_sequence_ = new TagSequence(1000);
  self->crf_tagger_ = CRFTagger::Create(model_path, self->tag_set_);
  self->feature_extractor_ = new SegmentFeatureExtractor();

  if (self->crf_tagger_ == NULL) {
    delete self;
    return NULL;
  }

  return self;
}

CRFSegmenter::~CRFSegmenter() {
  if (tag_set_ != NULL) {
    delete tag_set_;
    tag_set_ = NULL;
  }

  if (feature_extractor_ != NULL) {
    delete feature_extractor_;
    feature_extractor_ = NULL;
  }
  
  if (crf_tagger_ != NULL) {
    delete crf_tagger_;
    crf_tagger_ = NULL;
  }

  if (tag_sequence_ != NULL) {
    delete tag_sequence_;
    tag_sequence_ = NULL;
  }
}

CRFSegmenter::CRFSegmenter(): tag_set_(NULL), 
                              crf_tagger_(NULL), 
                              tag_sequence_(NULL),
                              feature_extractor_(NULL) {}

void CRFSegmenter::Segment(TermInstance *term_instance, const TokenInstance *token_instance) {
  std::string buffer;

  feature_extractor_->set_token_instance(token_instance);
  crf_tagger_->Tag(feature_extractor_, tag_sequence_);
  assert(token_instance->size() == tag_sequence_->length());

  TagSet::TagId tag_id;
  int term_count = 0;
  size_t i = 0;
  int token_count = 0;
  int term_type;
  for (i = 0; i < tag_sequence_->length(); ++i) {
    token_count++;
    buffer.append(token_instance->token_text_at(i));

    tag_id = tag_sequence_->GetTagAt(i);
    // printf("%s\n", tag_set_->TagIdToTagString(tag_id));
    if (tag_id == SegmentTagSet::S || tag_id == SegmentTagSet::E) {

      if (tag_id == SegmentTagSet::S) {
        term_type = token_type_to_word_type(token_instance->token_type_at(i));
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