//
// hmm_segment_and_pos_tagger.h --- Created at 2013-08-14
// bigram_segmenter.h --- Created at 2013-10-21
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

#ifndef BIGRAM_SEGMENTER_H
#define BIGRAM_SEGMENTER_H

#include <tr1/unordered_map>
#include <stdint.h>
#include "milkcat_config.h"
#include "darts.h"

class TokenInstance;
class TermInstance;

class BigramSegmenter {
 public:

  // A node in decode graph
  struct Node;

  // A Bucket contains several node
  class Bucket;

  // A pool to alloc and release nodes 
  class NodePool;

  static BigramSegmenter *Create(const char *trietree_path,
                                 const char *unigram_binary_path,
                                 const char *bigram_binary_path);

  ~BigramSegmenter();

  // Segment a token instance into term instance
  void Process(TermInstance *term_instance, const TokenInstance *token_instance);

 private:
  // Number of Node in each buckets_
  static const int kNBest = 3;

  // Buckets contain nodes for viterbi decoding
  Bucket *buckets_[kTokenMax + 1];

  // Index for words in dictionary
  Darts::DoubleArray *unigram_trie_;

  // Weight for each unigram term (Index by term_id)
  float *unigram_weight_;

  // Weight for bigram term_id pair, key is left_id << 32 + right_id
  std::tr1::unordered_map<int64_t, float> bigram_weight_;

  // NodePool instance to alloc and release node
  NodePool *node_pool_;

  BigramSegmenter();

  // Add an arc to the decode graph with the weight and term_id
  void AddArcToDecodeGraph(int from_position, int from_index, int to_position, double weight, int term_id);
};

#endif