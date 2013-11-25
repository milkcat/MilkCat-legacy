//
// crf_pos_tagger.h --- Created at 2013-08-20
// crf_part_of_speech_tagger.h --- Created at 2013-11-24
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


#ifndef CRF_PART_OF_SPEECH_TAGGER_H
#define CRF_PART_OF_SPEECH_TAGGER_H

#include "term_instance.h"
#include "part_of_speech_tag_instance.h"
#include "crf_tagger.h"
#include "milkcat_config.h" 
#include "part_of_speech_tagger.h"

class FeatureExtractor;
class PartOfSpeechFeatureExtractor;

class CRFPartOfSpeechTagger: public PartOfSpeechTagger {
 public:
  static CRFPartOfSpeechTagger *Create(const char *model_path);
  ~CRFPartOfSpeechTagger();

  // Tag the TermInstance and put the result to PartOfSpeechTagInstance
  void Tag(PartOfSpeechTagInstance *part_of_speech_tag_instance, TermInstance *term_instance) {
    TagRange(part_of_speech_tag_instance, term_instance, 0, term_instance->size());
  }

  // Tag a range [begin, end) of TermInstance and put the result to PartOfSpeechTagInstance
  void TagRange(PartOfSpeechTagInstance *part_of_speech_tag_instance, TermInstance *term_instance, int begin, int end);

 private:
  CRFTagger *crf_tagger_;
  CRFModel *crf_model_;

  CRFPartOfSpeechTagger();

  // Use FeatureExtractor to extract part-of-speech tagging features from TermInstance
  // to the CRF tagger
  PartOfSpeechFeatureExtractor *feature_extractor_;
  
  DISALLOW_COPY_AND_ASSIGN(CRFPartOfSpeechTagger);
};



#endif