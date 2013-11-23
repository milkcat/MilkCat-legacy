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
#include "crf_pos_tagger.h"
#include "hmm_part_of_speech_tagger.h"
#include "term_instance.h"
#include "part_of_speech_tag_instance.h"

MixedPartOfSpeechTagger::MixedPartOfSpeechTagger(CRFPOSTagger *crf_tagger, 
                                                 HMMPartOfSpeechTagger *hmm_tagger):
    crf_tagger_(crf_tagger), 
    hmm_tagger_(hmm_tagger) {

  one_instance_ = new PartOfSpeechTagInstance();
}

MixedPartOfSpeechTagger::~MixedPartOfSpeechTagger() {
  delete one_instance_;
}

void MixedPartOfSpeechTagger::Tag(PartOfSpeechTagInstance *part_of_speech_tag_instance, 
                                  TermInstance *term_instance) {
  hmm_tagger_->Tag(part_of_speech_tag_instance, term_instance);
  for (int i = 0; i < term_instance->size(); ++i) {
    if (term_instance->term_id_at(i) < 0 && term_instance->term_type_at(i) == TermInstance::kChineseWord) {
      crf_tagger_->TagRange(one_instance_, term_instance, i, i + 1);
      part_of_speech_tag_instance->set_value_at(i, one_instance_->part_of_speech_tag_at(0));
    }
  }
}