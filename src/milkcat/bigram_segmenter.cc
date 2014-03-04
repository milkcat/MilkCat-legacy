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
// hmm_segment_and_pos_tagger.cc --- Created at 2013-08-15
// bigram_segmenter.cc --- Created at 2013-10-23
//

#include "milkcat/bigram_segmenter.h"
#include <stdio.h>
#include <stdint.h>
#include <algorithm>
#include <vector>
#include <string>
#include "utils/utils.h"
#include "milkcat/darts.h"
#include "milkcat/libmilkcat.h"
#include "milkcat/milkcat_config.h"
#include "milkcat/term_instance.h"
#include "milkcat/token_instance.h"
#include "milkcat/trie_tree.h"

namespace milkcat {

struct BigramSegmenter::Node {
  // The bucket contains this node
  int bucket_id;

  // term_id for this node
  int term_id;

  // Weight summation in this path
  double weight;

  // Previous node pointer
  const Node *from_node;

  // Position in a term_instance
  int term_position;
};

namespace {

// Compare two Node in weight
static inline bool NodePtrCmp(BigramSegmenter::Node *n1,
                              BigramSegmenter::Node *n2) {
  return n1->weight < n2->weight;
}

}  // namespace

BigramSegmenter::BigramSegmenter(): beam_size_(0),
                                    node_pool_(nullptr),
                                    unigram_cost_(nullptr),
                                    user_cost_(nullptr),
                                    bigram_cost_(nullptr),
                                    index_(nullptr),
                                    user_index_(nullptr),
                                    has_user_index_(false),
                                    use_disabled_term_ids_(false) {
}

BigramSegmenter::~BigramSegmenter() {
  delete node_pool_;
  node_pool_ = nullptr;

  for (int i = 0; i < beams_.size(); ++i) {
    delete beams_[i];
    beams_[i] = nullptr;
  }
}

BigramSegmenter *BigramSegmenter::New(ModelFactory *model_factory,
                                      bool use_bigram,
                                      Status *status) {
  BigramSegmenter *self = new BigramSegmenter();

  self->beam_size_ = use_bigram? kDefaultBeamSize: 1;
  self->node_pool_ = new NodePool<Node>();

  // Initialize the beams_
  for (int i = 0; i < self->beams_.size(); ++i) {
    self->beams_[i] = new Beam<Node>(self->beam_size_,
                                     self->node_pool_,
                                     i,
                                     NodePtrCmp);
  }

  self->index_ = model_factory->Index(status);
  if (status->ok() && model_factory->HasUserDictionary()) {
    self->has_user_index_ = true;
    self->user_index_ = model_factory->UserIndex(status);
    if (status->ok()) self->user_cost_ = model_factory->UserCost(status);
  }

  if (status->ok()) self->unigram_cost_ = model_factory->UnigramCost(status);
  if (status->ok() && use_bigram == true) 
    self->bigram_cost_ = model_factory->BigramCost(status);

  // Default is not use disabled term-ids
  self->use_disabled_term_ids_ = false;

  if (status->ok()) {
    return self;
  } else {
    delete self;
    return nullptr;
  }
}

// Traverse the system and user index to find the term_id at current position,
// then get the unigram cost for the term-id. Return the term-id and stores the
// cost in double &unigram_cost
// NOTE: If the word in current position exists both in system dictionary and
// user dictionary, returns the term-id in system dictionary and stores the cost
// of user dictionary into unigram_cost if its value is not kDefaultCost
inline int BigramSegmenter::GetTermIdAndUnigramCost(
    const char *token_str,
    bool *system_flag,
    bool *user_flag,
    size_t *system_node,
    size_t *user_node,
    double *right_cost) {

  int term_id = TrieTree::kNone,
      uterm_id = TrieTree::kNone;

  if (*system_flag) {
    term_id = index_->Traverse(token_str, system_node);
    if (term_id == TrieTree::kNone) *system_flag = false;
    if (term_id >= 0) {
      *right_cost = unigram_cost_->get(term_id);
      // printf("system unigram find %d %lf\n", term_id, right_cost);
    }
  }

  if (*user_flag) {
    uterm_id = user_index_->Traverse(token_str, user_node);
    if (uterm_id == TrieTree::kNone) *user_flag = false;

    if (uterm_id >= 0) {
      double cost = user_cost_->get(uterm_id - kUserTermIdStart);
      // printf("user unigram find %d %lf\n", uterm_id, cost);
      if (term_id < 0) {
        *right_cost = cost;

        // Use term id in user dictionary iff term-id in system dictionary not
        // exist
        term_id = uterm_id;
      } else if (cost != kDefaultCost) {
        // If word exists in system dictionary only when the cost in user
        // dictionary not equals kDefaultCost, stores the cost to right_cost
        *right_cost = cost;
      }
    }
  }

  // If term-id in disabled list, set term_id to TrieTree::kNone
  if (use_disabled_term_ids_ == true) {
    if (disabled_term_ids_.find(term_id) != disabled_term_ids_.end()) {
      term_id = TrieTree::kNone;
    }
  }

  return term_id;
}

// Calculates the cost form left word-id to right term-id in bigram model. The
// cost equals -log(p(right_word|left_word)). If no bigram data exists, use
// unigram model cost = -log(p(right_word))
inline double BigramSegmenter::CalculateBigramCost(int left_id,
                                                   int right_id,
                                                   double left_cost,
                                                   double right_cost) {
  double cost;

  // If bigram is disabled
  if (bigram_cost_ == nullptr) return left_cost + right_cost;

  int64_t key = (static_cast<int64_t>(left_id) << 32) + right_id;
  const float *it = bigram_cost_->Find(key);
  if (it != nullptr) {
    // if have bigram data use p(x_n+1|x_n) = p(x_n+1, x_n) / p(x_n)
    cost = left_cost + (*it - unigram_cost_->get(left_id));
    // printf("bigram find %d %d %lf\n", left_id, right_id, cost - left_cost);
  } else {
    cost = left_cost + right_cost;
  }

  return cost;
}

int BigramSegmenter::GetTermId(const char *term_str) {
  bool system_flag = true;
  bool user_flag = has_user_index_;
  size_t system_node = 0;
  size_t user_node = 0;
  double cost = 0.0;

  return GetTermIdAndUnigramCost(term_str,
                                 &system_flag,
                                 &user_flag,
                                 &system_node,
                                 &user_node,
                                 &cost);
}

void BigramSegmenter::Segment(TermInstance *term_instance,
                              TokenInstance *token_instance) {
  Node *new_node = node_pool_->Alloc();
  new_node->bucket_id = 0;
  new_node->term_id = 0;
  new_node->weight = 0;
  new_node->from_node = nullptr;
  new_node->term_position = -1;
  // Add begin-of-sentence node
  beams_[0]->Add(new_node);

  // Strat decoding
  size_t index_node, user_node;
  bool index_flag, user_flag;
  double weight, right_cost;
  const Node *node = nullptr;
  for (int bucket_id = 0; bucket_id < token_instance->size(); ++bucket_id) {
    // Shrink current bucket to ensure node number < n_best
    beams_[bucket_id]->Shrink();

    index_node = 0;
    user_node = 0;
    index_flag = true;
    user_flag = has_user_index_;
    for (int bucket_count = 0;
         bucket_count + bucket_id < token_instance->size();
         ++bucket_count) {
      // printf("pos: %d %d\n", bucket_id, bucket_count);
      // Get current term-id from system and user dictionary
      const char *token_str = token_instance->token_text_at(
          bucket_id + bucket_count);
      int term_id = GetTermIdAndUnigramCost(token_str,
                                            &index_flag,
                                            &user_flag,
                                            &index_node,
                                            &user_node,
                                            &right_cost);

      double min_weight = 100000000;
      const Node *min_node = NULL;

      assert(beams_[bucket_id]->size() > 0);

      if (term_id >= 0) {
        // This token exists in unigram data
        for (int node_id = 0;
             node_id < beams_[bucket_id]->size();
             ++node_id) {
          node = beams_[bucket_id]->node_at(node_id);
          double weight = CalculateBigramCost(node->term_id,
                                              term_id,
                                              node->weight,
                                              right_cost);
          // printf("final cost is %lf\n", weight - node->weight);
          if (weight < min_weight) {
            min_weight = weight;
            min_node = node;
          }
        }

        // Add the min_node to decode graph
        new_node = node_pool_->Alloc();
        new_node->bucket_id = bucket_id + bucket_count + 1;
        new_node->term_id = term_id;
        new_node->weight = min_weight;
        new_node->from_node = min_node;
        new_node->term_position = min_node->term_position + 1;
        // Add begin-of-sentence node
        beams_[bucket_id + bucket_count + 1]->Add(new_node);
      } else {
        // One token out-of-vocabulary word should be always put into Decode
        // Graph When no arc to next bucket
        if (bucket_count == 0 && beams_[bucket_id + 1]->size() == 0) {
          for (int node_id = 0;
               node_id < beams_[bucket_id]->size();
               ++node_id) {
            node = beams_[bucket_id]->node_at(node_id);
            weight = node->weight + 20;

            if (weight < min_weight) {
              min_weight = weight;
              min_node = node;
            }
          }

          new_node = node_pool_->Alloc();
          new_node->bucket_id = bucket_id + bucket_count + 1;
          new_node->term_id = 0;
          new_node->weight = min_weight;
          new_node->from_node = min_node;
          new_node->term_position = min_node->term_position + 1;
          // Add begin-of-sentence node
          beams_[bucket_id + 1]->Add(new_node);
        }  // end if node count == 0
      }  // end if term_id >= 0

      if (index_flag == false && user_flag == false) break;
    }  // end for bucket_count
  }  // end for decode_start

  // Find the best result from decoding graph
  node = beams_[token_instance->size()]->MinimalNode();

  // Set the cost data for RecentSegCost()
  cost_ = node->weight;

  term_instance->set_size(node->term_position + 1);
  int bucket_id, from_bucket_id, term_type;
  std::string buffer;
  while (node->term_position >= 0) {
    buffer.clear();
    bucket_id = node->bucket_id;
    from_bucket_id = node->from_node->bucket_id;

    for (int i = from_bucket_id; i < bucket_id; ++i) {
      buffer.append(token_instance->token_text_at(i));
    }

    term_type = bucket_id - from_bucket_id > 1?
        TermInstance::kChineseWord:
        TokenTypeToTermType(token_instance->token_type_at(from_bucket_id));

    int out_of_vocabulary_id = TermInstance::kTermIdOutOfVocabulary;
    term_instance->set_value_at(
        node->term_position,
        buffer.c_str(),
        bucket_id - from_bucket_id,
        term_type,
        node->term_id == 0? out_of_vocabulary_id: node->term_id);
    node = node->from_node;
  }  // end while

  // Clear decode_node
  for (int i = 0; i < token_instance->size() + 1; ++i) {
    beams_[i]->Clear();
  }
  node_pool_->ReleaseAll();
}

}  // namespace milkcat
