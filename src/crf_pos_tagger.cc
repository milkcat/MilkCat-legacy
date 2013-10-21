/*
 * crf_pos_tagger.cc
 *
 * by ling0322 at 2013-10-09
 *
 */

#include <string.h>
#include "crf_pos_tagger.h"
#include "feature_extractor.h"

class PartOfSpeechFeatureExtractor: public FeatureExtractor {
 public:
  PartOfSpeechFeatureExtractor(): FeatureExtractor(3) {
  }

  void set_term_instance(const TermInstance *term_instance) { term_instance_ = term_instance; }
  size_t size() const { return term_instance_->size(); }

  const char **ExtractFeatureAt(size_t position) {
    int term_type = term_instance_->term_type_at(position);
    const char *term_text = term_instance_->term_text_at(position);
    size_t term_length = strlen(term_text);

    switch (term_type) {
     case TermInstance::kChineseWord:
      // term itself
      strcpy(feature_list_[0], term_text);     

      // first character of the term
      strncpy(feature_list_[1], term_text, 3);
      feature_list_[1][4] = 0;

      // last character of the term
      strcpy(feature_list_[2], term_text + term_length - 3);
      break;

     case TermInstance::kEnglishWord:
     case TermInstance::kSymbol:
      strcpy(feature_list_[0], "A");   
      strcpy(feature_list_[1], "A");
      strcpy(feature_list_[2], "A");
      break;

     case TermInstance::kNumber:
      strcpy(feature_list_[0], "1");   
      strcpy(feature_list_[1], "1");
      strcpy(feature_list_[2], "1");
      break;

     default:
      strcpy(feature_list_[0], ".");   
      strcpy(feature_list_[1], ".");
      strcpy(feature_list_[2], ".");
      break;     
    }

    return const_cast<const char **>(feature_list_);
  }


 private:
  const TermInstance *term_instance_;
};



CRFPOSTagger *CRFPOSTagger::Create(const char *model_path) {
  CRFPOSTagger *self = new CRFPOSTagger();

  self->pos_tag_set_ = new POSTagSet();
  self->tag_sequence_ = new TagSequence(kTokenMax);
  self->feature_extractor_ = new PartOfSpeechFeatureExtractor();

  self->crf_tagger_ = CRFTagger::Create(model_path, self->pos_tag_set_);
  if (self->crf_tagger_ == NULL) {
    delete self;
    return NULL;
  }

  return self;
}

CRFPOSTagger::CRFPOSTagger(): pos_tag_set_(NULL), 
                              crf_tagger_(NULL), 
                              tag_sequence_(NULL),
                              feature_extractor_(NULL) {}

CRFPOSTagger::~CRFPOSTagger() {
  if (pos_tag_set_ != NULL) {
    delete pos_tag_set_;
    pos_tag_set_ = NULL;
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

void CRFPOSTagger::Tag(PartOfSpeechTagInstance *part_of_speech_tag_instance, const TermInstance *term_instance) {
  feature_extractor_->set_term_instance(term_instance);
  crf_tagger_->Tag(feature_extractor_, tag_sequence_);
  for (size_t i = 0; i < term_instance->size(); ++i) {
    part_of_speech_tag_instance->set_value_at(i, pos_tag_set_->TagIdToTagString(tag_sequence_->GetTagAt(i))); 
  }
  part_of_speech_tag_instance->set_size(term_instance->size());
}