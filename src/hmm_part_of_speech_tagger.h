//
// hmm_part_of_speech_tagger.h --- Created at 2013-11-08
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

#ifndef HMM_PART_OF_SPEECH_TAGGER_H
#define HMM_PART_OF_SPEECH_TAGGER_H

#include <darts.h>
#include "milkcat_config.h"

class PartOfSpeechTagInstance;
class TermInstance;
class Configuration;

class HMMPartOfSpeechTagger {
 public:
  static const int kMaxBucket = kTokenMax;

  ~HMMPartOfSpeechTagger();
  void Tag(PartOfSpeechTagInstance *part_of_speech_tag_instance, TermInstance *term_instance);

  static HMMPartOfSpeechTagger *Create(const char *model_path, 
                                       const char *index_path,
                                       const char *default_tag_path);

 private:
  struct TermTagProbability;
  struct Node;

  Node *buckets_[kMaxBucket];
  TermTagProbability *term_tags_[kMaxBucket];
  TermTagProbability **emit_matrix_;

  // The emit linklist with only oen tag
  TermTagProbability **one_tag_emit_;

  // the the default emit tag for term type
  int term_type_emit_tag_[6];

  char (* tag_str_)[16];
  int tag_num_;
  int max_term_id_;
  double *transition_matrix_;
  Darts::DoubleArray unigram_trie_;

  HMMPartOfSpeechTagger();

  void CalculateBucketCost(int position);
  void CalculateArcCost(int position);

  // Get the tag's id by its text if the tag not exists return -1
  int GetTagIdByStr(const char *tag_str);

  // Load the default tag key from configuration file and put in term_type_emit_tag_
  void LoadDefaultTags(const Configuration *conf, const char *key, int *emit_tag);

  // Get each term's emit tag and save it in term_tags_
  void BuildEmitTagfForNode(TermInstance *term_instance);

};


#endif