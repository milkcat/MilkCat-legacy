//
// crf_pos_tagger.cc --- Created at 2013-10-09
// crf_part_of_speech_tagger.cc --- Created at 2013-11-24
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

#include <string.h>
#include "crf_part_of_speech_tagger.h"
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
      strlcpy(feature_list[0], term_text, kFeatureLengthMax);     

      // first character of the term
      strlcpy(feature_list[1], term_text, 4);

      // last character of the term
      strlcpy(feature_list[2], term_text + term_length - 3, kFeatureLengthMax);
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



CRFPartOfSpeechTagger *CRFPartOfSpeechTagger::Create(const char *model_path) {
  CRFPartOfSpeechTagger *self = new CRFPartOfSpeechTagger();

  self->feature_extractor_ = new PartOfSpeechFeatureExtractor();
  self->crf_model_ = CRFModel::Create(model_path);
  if (self->crf_model_ == NULL) {
    delete self;
    return NULL;
  }

  self->crf_tagger_ = new CRFTagger(self->crf_model_);

  return self;
}

CRFPartOfSpeechTagger::CRFPartOfSpeechTagger(): crf_tagger_(NULL), 
                                                crf_model_(NULL),
                                                feature_extractor_(NULL) {
}

CRFPartOfSpeechTagger::~CRFPartOfSpeechTagger() {
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

void CRFPartOfSpeechTagger::TagRange(PartOfSpeechTagInstance *part_of_speech_tag_instance, 
                                     TermInstance *term_instance,
                                     int begin,
                                     int end) {
  feature_extractor_->set_term_instance(term_instance);
  crf_tagger_->TagRange(feature_extractor_, begin, end);
  for (size_t i = 0; i < end - begin; ++i) {
    part_of_speech_tag_instance->set_value_at(i, crf_tagger_->GetTagText(crf_tagger_->GetTagAt(i))); 
  }
  part_of_speech_tag_instance->set_size(end - begin);
}