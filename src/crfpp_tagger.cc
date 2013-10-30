//
// crfpp_tagger.cc --- Created at 2013-10-30
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

#include <string>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "crfpp_tagger.h"
#include "utils.h"


struct CrfppTagger::Node {
  double cost;
  int left_tag_id;
};

const size_t kMaxContextSize = 8;
const char *BOS[kMaxContextSize] = { "_B-1", "_B-2", "_B-3", "_B-4",
                                     "_B-5", "_B-6", "_B-7", "_B-8" };
const char *EOS[kMaxContextSize] = { "_B+1", "_B+2", "_B+3", "_B+4",
                                     "_B+5", "_B+6", "_B+7", "_B+8" };


CrfppTagger::CrfppTagger(CrfppModel *model): model_(model) {
  for (int i = 0; i < kMaxBucket; ++i) {
    buckets_[i] = new Node[model_->GetTagNumber()];
  }
}

CrfppTagger::~CrfppTagger() {
  for (int i = 0; i < kMaxBucket; ++i) {
    delete buckets_[i];
  }
}

void CrfppTagger::TagRange(FeatureExtractor *feature_extractor, int begin, int end) {
  feature_extractor_ = feature_extractor;
  Viterbi(begin, end);
  FindBestResult(begin, end);
}

void CrfppTagger::Viterbi(int begin, int end) {
  assert(begin >= 0 && begin < end && end <= feature_extractor_->size());
  ClearBucket(begin);
  CalculateBucketCost(begin);
  for (int position = begin + 1; position < end; ++position) {
    ClearBucket(position);
    CalculateArcCost(position);
    CalculateBucketCost(position);
  }
}

void CrfppTagger::FindBestResult(int begin, int end) {
  int best_tag_id = 0;
  double best_cost = -1e37;
  const Node *last_bucket = buckets_[end - 1];
  for (int tag_id = 0; tag_id < model_->GetTagNumber(); ++tag_id) {
    if (best_cost < last_bucket[tag_id].cost) {
      best_tag_id = tag_id;
      best_cost = last_bucket[tag_id].cost;
    }
  }

  for (int position = end - 1; position >= begin; --position) {
    result_[position - begin] = best_tag_id;
    best_tag_id = buckets_[position][best_tag_id].left_tag_id;
  }
}

void CrfppTagger::ClearBucket(int position) {
  memset(buckets_[position], 0, sizeof(Node) * model_->GetTagNumber());
}

void CrfppTagger::CalculateBucketCost(int position) {
  int feature_ids[kMaxFeature],
      feature_id;
  int feature_num = GetUnigramFeatureIds(position, feature_ids);
  double cost;

  for (int tag_id = 0; tag_id < model_->GetTagNumber(); ++tag_id) {
    cost = buckets_[position][tag_id].cost;
    for (int i = 0; i < feature_num; ++i) {
      feature_id = feature_ids[i];
      cost += model_->GetUnigramCost(feature_id, tag_id);
    }
    buckets_[position][tag_id].cost = cost;
  }
}

void CrfppTagger::CalculateArcCost(int position) {
  int feature_ids[kMaxFeature],
      feature_id;
  int feature_num = GetBigramFeatureIds(position, feature_ids);
  double cost, best_cost; 
  int best_tag_id;

  for (int tag_id = 0; tag_id < model_->GetTagNumber(); ++tag_id) {
    best_cost = -1e37;
    for (int left_tag_id = 0; left_tag_id < model_->GetTagNumber(); ++left_tag_id) {
      cost = buckets_[position - 1][left_tag_id].cost;
      for (int i = 0; i < feature_num; ++i) {
        feature_id = feature_ids[i];
        cost += model_->GetBigramCost(feature_id, left_tag_id, tag_id);
      }
      if (cost > best_cost) {
        best_tag_id = left_tag_id;
        best_cost = cost;
      }
    }
    buckets_[position][tag_id].cost = best_cost;
    buckets_[position][tag_id].left_tag_id = best_tag_id;
  }
}

int CrfppTagger::GetUnigramFeatureIds(int position, int *feature_ids) {
  const char *template_str;
  int count = 0,
      feature_id;
  std::string feature_str;
  bool result;

  for (int i = 0; i < model_->UnigramTemplateNum(); ++i) {
    template_str = model_->GetUnigramTemplate(i);
    result = ApplyRule(feature_str, template_str, position);
    assert(result);
    feature_id = model_->GetFeatureId(feature_str.c_str());
    if (feature_id != -1) {
      printf("%s %d\n", feature_str.c_str(), feature_id);
      feature_ids[count++] = feature_id;
    }
  }

  return count;
}

int CrfppTagger::GetBigramFeatureIds(int position, int *feature_ids) {
  const char *template_str;
  int count = 0,
      feature_id;
  std::string feature_str;
  bool result;

  for (int i = 0; i < model_->BigramTemplateNum(); ++i) {
    template_str = model_->GetBigramTemplate(i);
    result = ApplyRule(feature_str, template_str, position);
    assert(result);
    feature_id = model_->GetFeatureId(feature_str.c_str());
    if (feature_id != -1) {
      
      feature_ids[count++] = feature_id;
    }
  }

  return count;
}

const char *CrfppTagger::GetIndex(const char *&p, int position) const {
  if (*p++ !='[') {
    return 0;
  }

  int col = 0;
  int row = 0;

  int neg = 1;
  if (*p++ == '-') {
    neg = -1;
  } else {
    --p;
  }

  for (; *p; ++p) {
    switch (*p) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        row = 10 * row +(*p - '0');
        break;
      case ',':
        ++p;
        goto NEXT1;
      default: return  0;
    }
  }

NEXT1:
  for (; *p; ++p) {
    switch (*p) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        col = 10 * col + (*p - '0');
        break;
      case ']': goto NEXT2;
      default: return 0;
    }
  }

NEXT2:
  row *= neg;

  if (row < -static_cast<int>(kMaxContextSize) ||
      row > static_cast<int>(kMaxContextSize) ||
      col < 0 || col >= static_cast<int>(model_->xsize())) {
    return 0;
  }

  const int idx = position + row;
  if (idx < 0) {
    return BOS[-idx-1];
  }
  if (idx >= static_cast<int>(feature_extractor_->size())) {
    return EOS[idx - feature_extractor_->size()];
  }

  return feature_extractor_->ExtractFeatureAt(idx)[col];
}

bool CrfppTagger::ApplyRule(std::string &output_str, const char *template_str, size_t position) const {
  const char *p = template_str,
             *index_str;

  output_str.clear();
  for (; *p; p++) {
    switch (*p) {
      default:
        output_str.push_back(*p);
        break;
      case '%':
        switch (*++p) {
          case 'x':
            ++p;
            index_str = GetIndex(p, position);
            if (!index_str) {
              return false;
            }
            output_str.append(index_str);
            break;
          default:
            return false;
        }
        break;
    }
  }

  return true;
}