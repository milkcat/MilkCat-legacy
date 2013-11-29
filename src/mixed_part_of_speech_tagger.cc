//
// mixed_part_of_speech_tagger.cc --- Created at 2013-11-23
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


#include "mixed_part_of_speech_tagger.h"
#include "crf_part_of_speech_tagger.h"
#include "hmm_part_of_speech_tagger.h"
#include "term_instance.h"
#include "part_of_speech_tag_instance.h"

MixedPartOfSpeechTagger::MixedPartOfSpeechTagger(): crf_tagger_(NULL), hmm_tagger_(NULL), one_instance_(NULL) {}

MixedPartOfSpeechTagger *MixedPartOfSpeechTagger::Create(
    const char *hmm_pos_model_path,
    const char *unigram_index_path,
    const char *default_tag_path,
    const char *crf_pos_model_path) {

  MixedPartOfSpeechTagger *self = new MixedPartOfSpeechTagger();
  self->crf_tagger_ = CRFPartOfSpeechTagger::Create(crf_pos_model_path);
  if (self->crf_tagger_ == NULL) {
    delete self;
    return NULL;
  }

  self->hmm_tagger_ = HMMPartOfSpeechTagger::Create(hmm_pos_model_path,
                                                    unigram_index_path,
                                                    default_tag_path);
  if (self->hmm_tagger_ == NULL) {
    delete self;
    return NULL;
  }

  self->one_instance_ = new PartOfSpeechTagInstance();
  return self;
}

MixedPartOfSpeechTagger::~MixedPartOfSpeechTagger() {
  delete one_instance_;
  one_instance_ = NULL;

  delete crf_tagger_;
  crf_tagger_ = NULL;

  delete hmm_tagger_;
  hmm_tagger_ = NULL; 
}

void MixedPartOfSpeechTagger::Tag(PartOfSpeechTagInstance *part_of_speech_tag_instance, 
                                  TermInstance *term_instance) {
  hmm_tagger_->Tag(part_of_speech_tag_instance, term_instance);
  for (int i = 0; i < term_instance->size(); ++i) {

    // if the word is a oov word and type is a word
    if (part_of_speech_tag_instance->is_out_of_vocabulary_word_at(i) == true) {
      // puts(term_instance->term_text_at(i));
      crf_tagger_->TagRange(one_instance_, term_instance, i, i + 1);
      part_of_speech_tag_instance->set_value_at(i, one_instance_->part_of_speech_tag_at(0));
    }
  }
}