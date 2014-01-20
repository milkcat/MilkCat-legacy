//
// tag_segmenter.cc --- Created at 2013-02-12
// crf_segmenter.cc --- Created at 2013-08-17
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
      strcpy(feature_list[0], "，");
    }
  }

 private:
  const TokenInstance *token_instance_;
};

CRFSegmenter *CRFSegmenter::New(const CRFModel *model, Status &status) {
  char error_message[1024];
  CRFSegmenter *self = new CRFSegmenter();

  self->crf_tagger_ = new CRFTagger(model);
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
    status = Status::Corruption("bad CRF++ segmenter model, unable to find S, B, B1, B2, M, E tag.");
  }

  if (status.ok()) {
    return self;
  } else {
    delete self;
    return NULL;
  }
}

CRFSegmenter::~CRFSegmenter() {
  delete feature_extractor_;
  feature_extractor_ = NULL;
  
  delete crf_tagger_;
  crf_tagger_ = NULL;
}

CRFSegmenter::CRFSegmenter(): crf_tagger_(NULL), 
                              feature_extractor_(NULL) {}

void CRFSegmenter::SegmentRange(TermInstance *term_instance, TokenInstance *token_instance, int begin, int end) {
  std::string buffer;

  feature_extractor_->set_token_instance(token_instance);
  crf_tagger_->TagRange(feature_extractor_, begin, end, S, S);

  int tag_id;
  int term_count = 0;
  size_t i = 0;
  int token_count = 0;
  int term_type;
  for (i = 0; i < end - begin; ++i) {
    token_count++;
    buffer.append(token_instance->token_text_at(begin + i));

    tag_id = crf_tagger_->GetTagAt(i);
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