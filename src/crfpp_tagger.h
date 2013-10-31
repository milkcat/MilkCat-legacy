//
// crfpp_tagger.h --- Created at 2013-10-30
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
// Part of this code comes from CRF++
//
// CRF++ -- Yet Another CRF toolkit
// Copyright(C) 2005-2007 Taku Kudo <taku@chasen.org>
//

#ifndef CRFPP_TAGGER_H
#define CRFPP_TAGGER_H

#include <string>
#include "milkcat_config.h"
#include "crfpp_model.h"
#include "feature_extractor.h"

class CrfppTagger {
 public:
  CrfppTagger(CrfppModel *model);
  ~CrfppTagger();
  static const int kMaxBucket = kTokenMax;
  static const int kMaxFeature = 24;

  // Tag a range of instance
  void TagRange(FeatureExtractor *feature_extractor, int begin, int end);

  // Tag a sentence the result could retrive by GetTagAt
  void Tag(FeatureExtractor *feature_extractor) {
    TagRange(feature_extractor, 0, feature_extractor->size());
  }

  // Get the result tag at position, position starts from 0
  int GetTagAt(int position) {
    return result_[position];
  }

 private:
  struct Node;
  
  CrfppModel *model_;
  Node *buckets_[kMaxBucket];
  int result_[kMaxBucket];
  FeatureExtractor *feature_extractor_;

  char feature_cache_[kMaxBucket][kMaxFeature][kFeatureLengthMax];
  int feature_cache_left_;
  int feature_cache_right_;
  bool feature_cache_flag_[kMaxBucket];

  // Get the feature from cache or feature extractor
  const char *GetFeatureAt(int position, int index);

  // Clear the Feature cache
  void ClearFeatureCache();

  // Get the id list of bigram features in position, returns the size of the list
  int GetBigramFeatureIds(int position, int *feature_ids);

  // Get the id list of unigram features in position, returns the size of the list
  int GetUnigramFeatureIds(int position, int *feature_ids);

  // CLear the decode bucket
  void ClearBucket(int position);

  // Calculate the unigram cost for each tag in bucket
  void CalculateBucketCost(int position);

  // Calcualte the bigram cost from tag to tag in bucket
  void CalculateArcCost(int position);

  // Viterbi algorithm
  void Viterbi(int begin, int end);

  // Get the best tag sequence from Viterbi result
  void FindBestResult(int begin, int end);

  const char *GetIndex(const char *&p, int position);
  bool ApplyRule(std::string &output_str, const char *template_str, size_t position);
  
};


#endif