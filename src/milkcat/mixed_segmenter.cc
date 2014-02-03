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

MixedSegmenter *MixedSegmenter::New(
    const TrieTree *index,
    const TrieTree *user_index,
    const StaticArray<float> *unigram_cost,
    const StaticHashTable<int64_t, float> *bigram_cost,
    const CRFModel *seg_model,
    const TrieTree *oov_property,
    Status &status) {

  MixedSegmenter *self = new MixedSegmenter();
  self->bigram_ = new BigramSegmenter(index, user_index, unigram_cost, bigram_cost);
  self->oov_recognizer_ = OutOfVocabularyWordRecognition::New(seg_model, oov_property, status);
  if (status.ok()) self->bigram_result_ = new TermInstance();

  if (status.ok()) {
    return self;
  } else {
    delete self;
    return NULL;
  }
  
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