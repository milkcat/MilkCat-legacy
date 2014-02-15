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
#include "milkcat/milkcat_config.h"
#include "milkcat/token_instance.h"
#include "milkcat/term_instance.h"
#include "milkcat/trie_tree.h"

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

// Compare two Node in weight
static inline bool NodePtrCmp(const BigramSegmenter::Node *n1,
                              const BigramSegmenter::Node *n2) {
  return n1->weight < n2->weight;
}

class BigramSegmenter::NodePool {
 public:
  explicit NodePool(int capability): nodes_(capability), alloc_index_(0) {
    for (auto &node : nodes_) node = new Node();
  }

  ~NodePool() {
    for (auto &node : nodes_) delete node;
  }

  // Alloc a node
  Node *Alloc() {
    Node *node;
    if (alloc_index_ == nodes_.size()) {
      nodes_.push_back(new Node());
    }
    return nodes_[alloc_index_++];
  }

  // Release all node alloced before
  void ReleaseAll() {
    alloc_index_ = 0;
  }

 private:
  std::vector<Node *> nodes_;
  int alloc_index_;
};

class BigramSegmenter::Bucket {
 public:
  Bucket(int n_best, NodePool *node_pool, int bucket_id):
      n_best_(n_best),
      capability_(n_best * 3),
      bucket_id_(bucket_id),
      size_(0),
      node_pool_(node_pool) {
    nodes_ = new Node *[capability_];
  }

  ~Bucket() {
    delete[] nodes_;
  }

  int size() const { return size_; }
  const Node *node_at(int index) const { return nodes_[index]; }

  // Shrink nodes_ array and remain top n_best elements
  void Shrink() {
    if (size_ <= n_best_) return;
    std::partial_sort(nodes_, nodes_ + n_best_, nodes_ + size_, NodePtrCmp);
    size_ = n_best_;
  }

  // Clear all elements in bucket
  void Clear() {
    size_ = 0;
  }

  // Get min node in the bucket
  const Node *MinimalNode() {
    return *std::min_element(nodes_, nodes_ + size_, NodePtrCmp);
  }

  // Add an arc to decode graph
  void AddArc(const Node *from_node, double weight, int term_id) {
    nodes_[size_] = node_pool_->Alloc();
    nodes_[size_]->bucket_id = bucket_id_;
    nodes_[size_]->term_id = term_id;
    nodes_[size_]->weight = weight;
    nodes_[size_]->from_node = from_node;

    // for the begin-of-sentence which from_node == NULL
    if (from_node != NULL) {
      nodes_[size_]->term_position = from_node->term_position + 1;
    } else {
      nodes_[size_]->term_position = -1;
    }

    size_++;
    if (size_ >= capability_) Shrink();
  }

 private:
  Node **nodes_;
  int capability_;
  NodePool *node_pool_;
  int bucket_id_;
  int n_best_;
  int size_;
};

BigramSegmenter::~BigramSegmenter() {
  delete node_pool_;
  node_pool_ = NULL;

  for (int i = 0; i < sizeof(buckets_) / sizeof(Bucket *); ++i) {
    delete buckets_[i];
    buckets_[i] = NULL;
  }
}

BigramSegmenter::BigramSegmenter(
    const TrieTree *index,
    const TrieTree *user_index,
    const StaticArray<float> *unigram_cost,
    const StaticArray<float> *user_cost,
    const StaticHashTable<int64_t, float> *bigram_cost) {

  int64_t file_size;
  int record_number;

  node_pool_ = new NodePool(kNBest * kTokenMax);

  // Initialize the buckets_
  for (int i = 0; i < sizeof(buckets_) / sizeof(Bucket *); ++i) {
    buckets_[i] = new Bucket(kNBest, node_pool_, i);
  }

  index_ = index;
  user_index_ = user_index;
  unigram_cost_ = unigram_cost;
  user_cost_ = user_cost;
  bigram_cost_ = bigram_cost;

  has_user_index_ = (user_index_ != NULL);

  // Default is not use disabled term-ids
  use_disabled_term_ids_ = false;
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
  // Add begin-of-sentence node
  buckets_[0]->AddArc(NULL, 0.0, 0);

  // Strat decoding
  size_t index_node, user_node;
  bool index_flag, user_flag;
  const float *bigram_map_iter;
  double weight, right_cost;
  const Node *node;
  for (int bucket_id = 0; bucket_id < token_instance->size(); ++bucket_id) {
    // Shrink current bucket to ensure node number < n_best
    buckets_[bucket_id]->Shrink();

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

      assert(buckets_[bucket_id]->size() > 0);

      if (term_id >= 0) {
        // This token exists in unigram data
        for (int node_id = 0;
             node_id < buckets_[bucket_id]->size();
             ++node_id) {
          node = buckets_[bucket_id]->node_at(node_id);
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
        buckets_[bucket_id + bucket_count + 1]->AddArc(min_node,
                                                       min_weight,
                                                       term_id);

      } else {
        // One token out-of-vocabulary word should be always put into Decode
        // Graph When no arc to next bucket
        if (bucket_count == 0 && buckets_[bucket_id + 1]->size() == 0) {
          for (int node_id = 0;
               node_id < buckets_[bucket_id]->size();
               ++node_id) {
            node = buckets_[bucket_id]->node_at(node_id);
            weight = node->weight + 20;

            if (weight < min_weight) {
              min_weight = weight;
              min_node = node;
            }
          }

          // term_id = 0 for out-of-vocabulary word
          buckets_[bucket_id + 1]->AddArc(min_node, min_weight, 0);
        }  // end if node count == 0
      }  // end if term_id >= 0

      if (index_flag == false && user_flag == false) break;
    }  // end for bucket_count
  }  // end for decode_start

  // Find the best result from decoding graph
  node = buckets_[token_instance->size()]->MinimalNode();

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
    buckets_[i]->Clear();
  }
  node_pool_->ReleaseAll();
}
