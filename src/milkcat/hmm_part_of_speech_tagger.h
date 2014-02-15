//
// The MIT License (MIT)
//
// Copyright 2013-2014 The MilkCat Project Developers
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
// hmm_part_of_speech_tagger.h --- Created at 2013-11-08
//

#ifndef SRC_MILKCAT_HMM_PART_OF_SPEECH_TAGGER_H_
#define SRC_MILKCAT_HMM_PART_OF_SPEECH_TAGGER_H_

#include "utils/utils.h"
#include "milkcat/darts.h"
#include "milkcat/milkcat_config.h"
#include "milkcat/part_of_speech_tagger.h"
#include "milkcat/hmm_model.h"
#include "milkcat/trie_tree.h"
#include "milkcat/configuration.h"

namespace milkcat {

class PartOfSpeechTagInstance;
class TermInstance;
class Configuration;

class HMMPartOfSpeechTagger: public PartOfSpeechTagger {
 public:
  static const int kMaxBucket = kTokenMax;

  ~HMMPartOfSpeechTagger();
  void Tag(PartOfSpeechTagInstance *part_of_speech_tag_instance,
           TermInstance *term_instance);

  static HMMPartOfSpeechTagger *New(const HMMModel *model,
                                    const TrieTree *index,
                                    const Configuration *default_tag,
                                    Status *status);

 private:
  struct Node;

  Node *buckets_[kMaxBucket];
  HMMModel::EmitRow *term_tags_[kMaxBucket];
  const HMMModel *model_;
  int tag_num_;

  // If the word of position has hmm emit data
  bool has_data_[kMaxBucket];

  // The emit linklist with only one tag
  HMMModel::EmitRow **one_tag_emit_;

  // the the default emit tag for term type
  int term_type_emit_tag_[6];

  const TrieTree *index_;

  HMMPartOfSpeechTagger();

  // Calculate the emit cost in position
  void CalculateBucketCost(int position);

  // Calculate the transition cost from position - 1 to position
  void CalculateArcCost(int position);

  // Get the tag's id by its text if the tag not exists return -1
  int GetTagIdByStr(const char *tag_str);

  // Load the default tag key from configuration file and put in
  // term_type_emit_tag_
  void LoadDefaultTags(const Configuration *conf,
                       const char *key,
                       int *emit_tag,
                       Status *status);

  // Get each term's emit tag and save it in term_tags_
  void BuildEmitTagfForNode(TermInstance *term_instance);

  DISALLOW_COPY_AND_ASSIGN(HMMPartOfSpeechTagger);
};

}  // namespace milkcat

#endif  // SRC_MILKCAT_HMM_PART_OF_SPEECH_TAGGER_H_
