/*
 * crf_pos_tagger.cc
 *
 * by ling0322 at 2013-10-09
 *
 */

#include <string.h>
#include "crf_pos_tagger.h"
#include "feature_extractor.h"
#include "milkcat_config.h"

class PartOfSpeechFeatureExtractor: public FeatureExtractor {
 public:

  void set_term_instance(const TermInstance *term_instance) { term_instance_ = term_instance; }
  size_t size() const { return term_instance_->size(); }

  void ExtractFeatureAt(size_t position, char (*feature_list)[kFeatureLengthMax], int list_size) {
    assert(list_size == 3);
    int term_type = term_instance_->term_type_at(position);
    const char *term_text = term_instance_->term_text_at(position);
    size_t term_length = strlen(term_text);

    switch (term_type) {
     case TermInstance::kChineseWord:
      // term itself
      strcpy(feature_list[0], term_text);     

      // first character of the term
      strncpy(feature_list[1], term_text, 3);
      feature_list[1][4] = 0;

      // last character of the term
      strcpy(feature_list[2], term_text + term_length - 3);
      break;

     case TermInstance::kEnglishWord:
     case TermInstance::kSymbol:
      strcpy(feature_list[0], "A");   
      strcpy(feature_list[1], "A");
      strcpy(feature_list[2], "A");
      break;

     case TermInstance::kNumber:
      strcpy(feature_list[0], "1");   
      strcpy(feature_list[1], "1");
      strcpy(feature_list[2], "1");
      break;

     default:
      strcpy(feature_list[0], ".");   
      strcpy(feature_list[1], ".");
      strcpy(feature_list[2], ".");
      break;     
    }
  }
 private:
  const TermInstance *term_instance_;
};



CRFPOSTagger *CRFPOSTagger::Create(const char *model_path) {
  CRFPOSTagger *self = new CRFPOSTagger();

  self->feature_extractor_ = new PartOfSpeechFeatureExtractor();
  self->crf_model_ = CRFModel::Create(model_path);
  if (self->crf_model_ == NULL) {
    delete self;
    return NULL;
  }

  self->crf_tagger_ = new CRFTagger(self->crf_model_);

  return self;
}

CRFPOSTagger::CRFPOSTagger(): crf_tagger_(NULL), 
                              crf_model_(NULL),
                              feature_extractor_(NULL) {
}

CRFPOSTagger::~CRFPOSTagger() {
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

void CRFPOSTagger::TagRange(PartOfSpeechTagInstance *part_of_speech_tag_instance, 
                            const TermInstance *term_instance,
                            int begin,
                            int end) {
  feature_extractor_->set_term_instance(term_instance);
  crf_tagger_->TagRange(feature_extractor_, begin, end);
  for (size_t i = 0; i < end - begin; ++i) {
    part_of_speech_tag_instance->set_value_at(i, crf_tagger_->GetTagText(crf_tagger_->GetTagAt(i))); 
  }
  part_of_speech_tag_instance->set_size(end - begin);
}