//
// mixed_segmenter.cc --- Created at 2013-11-25
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

#include "mixed_segmenter.h"
#include "bigram_segmenter.h"
#include "out_of_vocabulary_word_recognition.h"
#include "term_instance.h"
#include "token_instance.h"

MixedSegmenter::MixedSegmenter(): bigram_(NULL), oov_recognizer_(NULL), bigram_result_(NULL) {}

MixedSegmenter *MixedSegmenter::Create(const char *unigram_index_path,
                                       const char *unigram_data_path,
                                       const char *bigram_data_path,
                                       const char *character_property_path,
                                       const char *crf_seg_model_path) {

  MixedSegmenter *self = new MixedSegmenter();
  self->bigram_ = BigramSegmenter::Create(unigram_index_path, unigram_data_path, bigram_data_path);
  if (self->bigram_ == NULL) {
    delete self;
    return NULL;
  }

  self->oov_recognizer_ = OutOfVocabularyWordRecognition::Create(crf_seg_model_path, character_property_path);
  if (self->oov_recognizer_ == NULL) {
    delete self;
    return NULL;
  }

  self->bigram_result_ = new TermInstance();
  return self;
}

MixedSegmenter::~MixedSegmenter() {
  delete bigram_result_;
  bigram_result_ = NULL;

  delete bigram_;
  bigram_ = NULL;

  delete oov_recognizer_;
  oov_recognizer_ = NULL;
}

void MixedSegmenter::Segment(TermInstance *term_instance, TokenInstance *token_instance) {
  bigram_->Segment(bigram_result_, token_instance);
  oov_recognizer_->Process(term_instance, bigram_result_, token_instance);
}