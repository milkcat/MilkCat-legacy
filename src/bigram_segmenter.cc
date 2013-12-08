//
// hmm_segment_and_pos_tagger.cc --- Created at 2013-08-15
// bigram_segmenter.cc --- Created at 2013-10-23
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

#include <stdio.h>
#include <darts.h>
#include <stdint.h>
#include <algorithm>
#include <iostream>
#include <string>
#include "bigram_segmenter.h"
#include "milkcat_config.h"
#include "token_instance.h"
#include "term_instance.h"
#include "utils.h"
#include "trie_tree.h"

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
static inline bool NodePtrCmp(const BigramSegmenter::Node *n1, const BigramSegmenter::Node *n2) {
  return n1->weight < n2->weight;
}

class BigramSegmenter::NodePool {
 public:
  NodePool(int capability): nodes_(capability), alloc_index_(0) {
    for (std::vector<Node *>::iterator it = nodes_.begin(); it != nodes_.end(); ++it) {
      *it = new Node();
    }
  }

  ~NodePool() {
    for (std::vector<Node *>::iterator it = nodes_.begin(); it != nodes_.end(); ++it) {
      delete *it;
    }
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
    if (size_ <= n_best_) return ;
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

BigramSegmenter::BigramSegmenter(const TrieTree *index,
                                 const StaticArray<float> *unigram_cost,
                                 const StaticHashTable<int64_t, float> *bigram_cost) {

  long file_size;
  int record_number;

  node_pool_ = new NodePool(kNBest * kTokenMax);

  // Initialize the buckets_
  for (int i = 0; i < sizeof(buckets_) / sizeof(Bucket *); ++i) {
    buckets_[i] = new Bucket(kNBest, node_pool_, i);
  }

  index_ = index;
  unigram_cost_ = unigram_cost;
  bigram_cost_ = bigram_cost;
}

void BigramSegmenter::Segment(TermInstance *term_instance, TokenInstance *token_instance) {

  // Add begin-of-sentence node
  buckets_[0]->AddArc(NULL, 0.0, 0);

  // Strat decoding
  size_t trie_node,
         key_position;
  int64_t left_term_id,
          right_term_id,
          left_right_id;
  const float *bigram_map_iter;
  double weight;
  const Node *node;
  for (int bucket_id = 0; bucket_id < token_instance->size(); ++bucket_id) {
    // Shrink current bucket to ensure node number < n_best
    buckets_[bucket_id]->Shrink();

    trie_node = 0;
    for (int bucket_count = 0; bucket_count + bucket_id < token_instance->size(); ++bucket_count) {
      // printf("%d %d\n", bucket_id, bucket_count);
      int term_id = index_->Traverse(token_instance->token_text_at(bucket_id + bucket_count), trie_node);

      double min_weight = 100000000;
      const Node *min_node = NULL;

      assert(buckets_[bucket_id]->size() > 0);

      if (term_id >= 0) {

        // This token exists in unigram data
        for (int node_id = 0; node_id < buckets_[bucket_id]->size(); ++node_id) {
          node = buckets_[bucket_id]->node_at(node_id);

          left_term_id = node->term_id;
          right_term_id = term_id;
          left_right_id = (left_term_id << 32) + right_term_id;

          bigram_map_iter = bigram_cost_->Find(left_right_id); 
          if (bigram_map_iter != NULL) {

            // if have bigram data use p(x_n+1|x_n) = p(x_n+1, x_n) / p(x_n)
            weight = node->weight + 0.7 * (*bigram_map_iter - unigram_cost_->get(left_term_id)) + 
                                    0.3 * unigram_cost_->get(right_term_id);
            // printf("bigram find %d %d %lf\n", bucket_id, bucket_count, weight);
          } else {

            // if np bigram data use p(x_n+1|x_n) = p(x_n+1)
            weight = node->weight + unigram_cost_->get(right_term_id);
            // printf("unigram find %d %d %lf\n", bucket_id, bucket_count, weight);
          }

          if (weight < min_weight) {
            min_weight = weight;
            min_node = node;
          }
        }

        // Add the min_node to decode graph
        buckets_[bucket_id + bucket_count + 1]->AddArc(min_node, min_weight, term_id);

      } else {
        // One token out-of-vocabulary word should be always put into Decode Graph
        // When no arc to next bucket
        if (bucket_count == 0 && buckets_[bucket_id + 1]->size() == 0) {
          for (int node_id = 0; node_id < buckets_[bucket_id]->size(); ++node_id) {
            node = buckets_[bucket_id]->node_at(node_id);
            weight = node->weight + 20;

            if (weight < min_weight) {
              min_weight = weight;
              min_node = node;
            }
          }

          // term_id = 0 for out-of-vocabulary word
          buckets_[bucket_id + 1]->AddArc(min_node, min_weight, 0);
        } // end if node count == 0

        if (term_id == TrieTree::kNone) break;
      } // end if term_id >= 0
    } // end for bucket_count
  } // end for decode_start

  // Find the best result from decoding graph
  node = buckets_[token_instance->size()]->MinimalNode();
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
    
    term_type = bucket_id - from_bucket_id > 1? TermInstance::kChineseWord: 
                                                token_type_to_word_type(token_instance->token_type_at(from_bucket_id));
    term_instance->set_value_at(node->term_position,
                                buffer.c_str(),
                                bucket_id - from_bucket_id,
                                term_type,
                                node->term_id == 0? TermInstance::kTermIdOutOfVocabulary: node->term_id);
    node = node->from_node;
  } // end while

  // Clear decode_node
  for (int i = 0; i < token_instance->size() + 1; ++i) {
    buckets_[i]->Clear();
  }
  node_pool_->ReleaseAll();

}

