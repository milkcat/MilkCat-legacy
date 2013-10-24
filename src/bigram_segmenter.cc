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

struct OptimalNode {
  // term_id for this node
  int term_id;

  // Weight summation in this path
  double weight;

  // Previous node position in decode_node_
  int previous_node_position;

  // Previous node index in the position of previous_node_position
  int previous_node_index;

  // Position in a term_instance
  int term_position;
};

// Record struct for bigram data
#pragma pack(1)
struct BigramRecord {
  int32_t left_id;
  int32_t right_id;
  float weight;
};
#pragma pack(0)

BigramSegmenter::BigramSegmenter(): unigram_trie_(NULL),
                                    unigram_weight_(NULL) {
  for (int i = 0; i < sizeof(decode_node_) / sizeof(OptimalNode *); ++i) {
    decode_node_[i] = NULL;
  }
}

BigramSegmenter::~BigramSegmenter() {
  if (unigram_trie_ != NULL) {
    delete unigram_trie_;
    unigram_trie_ = NULL;
  }

  if (unigram_weight_ != NULL) {
    delete[] unigram_weight_;
    unigram_weight_ = NULL;
  }

  for (int i = 0; i < sizeof(decode_node_) / sizeof(OptimalNode *); ++i) {
    if (decode_node_[i] != NULL) {
      delete[] decode_node_[i];
      decode_node_[i] = NULL;
    }
  }
}

BigramSegmenter *BigramSegmenter::Create(const char *trietree_path,
                                         const char *unigram_binary_path,
                                         const char *bigram_binary_path) {
  char error_message[1024];
  BigramSegmenter *self = new BigramSegmenter();
  FILE *fd;
  long file_size;
  int record_number;

  // Initialize the decode_node_
  for (int i = 0; i < sizeof(self->decode_node_) / sizeof(OptimalNode *); ++i) {
    self->decode_node_[i] = new OptimalNode[kNBest];
  }

  // Load unigram_trie file
  self->unigram_trie_ = new Darts::DoubleArray();
  if (-1 == self->unigram_trie_->open(trietree_path)) {
    sprintf(error_message, "unable to open index file %s", trietree_path);
    set_error_message(error_message);
    delete self;
    return NULL;
  }

  // Load unigram weight file
  fd = fopen(unigram_binary_path, "r");
  if (fd == NULL) { 
    sprintf(error_message, "unable to open unigram data file %s", unigram_binary_path);
    set_error_message(error_message);
    delete self;
    return NULL;    
  }

  fseek(fd, 0L, SEEK_END);
  file_size = ftell(fd);
  fseek(fd, 0L, SEEK_SET);
  record_number = file_size / sizeof(float);

  self->unigram_weight_ = new float[record_number];
  if (record_number != fread(self->unigram_weight_, sizeof(float), self->unigram_trie_->size(), fd)) {
    sprintf(error_message, "unable to read from unigram data file %s", unigram_binary_path);
    set_error_message(error_message);
    delete self;
    return NULL;      
  }

  fclose(fd);

  // Load bigram weight file
  fd = fopen(bigram_binary_path, "r");
  if (fd == NULL) {
    sprintf(error_message, "unable to open bigram data file %s", bigram_binary_path);
    set_error_message(error_message);
    delete self;
    return NULL;    
  }

  fseek(fd, 0L, SEEK_END);
  file_size = ftell(fd);
  fseek(fd, 0L, SEEK_SET);
  record_number = file_size / sizeof(BigramRecord);

  // load_factor 0.5 for unordered map
  self->bigram_weight_.rehash(record_number * 2); 
  BigramRecord bigram_record;
  for (int i = 0; i < record_number; ++i) {
    if (1 != fread(&bigram_record, sizeof(bigram_record), 1, fd)) {
      sprintf(error_message, "unable to read from bigram data file %s", bigram_binary_path);
      set_error_message(error_message);
      delete self;
      return NULL;         
    }
    self->bigram_weight_[(static_cast<int64_t>(bigram_record.left_id) << 32) + bigram_record.right_id] = bigram_record.weight;
  }

  fclose(fd);

  return self;
}

// Compare two weight value, 0 is greater than any other values
inline bool OptimalWeightCmp(double w1, double w2) {
  if (w1 == 0) return false;
  if (w2 == 0) return true;
  return w1 < w2; 
}

// Compare two OptimalNode in weight
bool OptimalNodeCmp(OptimalNode &n1, OptimalNode &n2) {
  return OptimalWeightCmp(n1.weight, n2.weight);
}

void BigramSegmenter::AddArcToDecodeGraph(int from_position, int from_index, int to_position, double weight, int term_id) {

  if (OptimalWeightCmp(weight, decode_node_[to_position][node_greatest_index[to_position]].weight) == true) {
    // printf("Arc from (%d, %d) to %d weight %f term_id %d\n", from_position, from_index, to_position, weight, term_id);
    OptimalNode &node = decode_node_[to_position][node_greatest_index[to_position]];
    node.term_id = term_id;
    node.weight = weight;
    node.previous_node_position = from_position;
    node.previous_node_index = from_index;
    node.term_position = decode_node_[from_position][from_index].term_position + 1;

    // Find the greatest weight value in node
    double max_weight = 0.1;
    int max_index = -1, i;
    for (i = 0; i < kNBest; ++i) {
      if (OptimalWeightCmp(max_weight, decode_node_[to_position][i].weight) == true) {
        max_weight = decode_node_[to_position][i].weight;
        max_index = i;
      }
    }
    node_greatest_index[to_position] = max_index;
    // printf("%d\n",node_greatest_index[to_position]);
  }
}

void BigramSegmenter::Process(TermInstance *term_instance, const TokenInstance *token_instance) {

  // Add begin-of-sentence node
  decode_node_[0][0].weight = 1;
  decode_node_[0][0].term_position = -1;

  // Strat decoding
  size_t double_array_node,
         key_position;
  int64_t left_term_id,
          right_term_id,
          left_right_id;
  std::tr1::unordered_map<int64_t, float>::const_iterator bigram_map_iter;
  double weight;
  for (int decode_start = 0; decode_start < token_instance->size(); ++decode_start) {
    double_array_node = 0;
    for (int node_count = 0; node_count + decode_start < token_instance->size(); ++node_count) {
      // printf("%d %d\n", decode_start, node_count);
      key_position = 0;
      int term_id = unigram_trie_->traverse(token_instance->token_text_at(decode_start + node_count), double_array_node, key_position);

      if (term_id >= 0) {

        // This token exists in unigram data
        for (int optimal_index = 0; optimal_index < kNBest; ++ optimal_index) {
          if (decode_node_[decode_start][optimal_index].weight == 0) continue;

          left_term_id = decode_node_[decode_start][optimal_index].term_id;
          right_term_id = term_id;
          left_right_id = (left_term_id << 32) + right_term_id;

          bigram_map_iter = bigram_weight_.find(left_right_id); 
          if (bigram_map_iter != bigram_weight_.end()) {

            // if have bigram data use p(x_n+1|x_n) = p(x_n+1, x_n) / p(x_n)
            weight = decode_node_[decode_start][optimal_index].weight + 
                     bigram_map_iter->second - unigram_weight_[left_term_id];
          } else {

            // if np bigram data use p(x_n+1|x_n) = p(x_n+1)
            weight = decode_node_[decode_start][optimal_index].weight + unigram_weight_[right_term_id];
          }
          AddArcToDecodeGraph(decode_start, optimal_index, decode_start + node_count + 1, weight, right_term_id);
        }

      } else {

        // One token out-of-vocabulary word should be always put into Decode Graph
        if (node_count == 0) {
          for (int optimal_index = 0; optimal_index < kNBest; ++ optimal_index) {
            // puts("233");
            if (decode_node_[decode_start][optimal_index].weight == 0) continue;
            weight = decode_node_[decode_start][optimal_index].weight + 20;
            AddArcToDecodeGraph(decode_start, optimal_index, decode_start + 1, weight, right_term_id);
          }
        } // end if node count == 0

        if (term_id == -2) break;
      } // end if term_id >= 0
    } // end for node_count
  } // end for decode_start

  // Find the best result from decoding graph
  OptimalNode *last = decode_node_[token_instance->size()];
  
  //
  double min_value = 0;
  int min_index = -1;
  for (int i = 0; i < kNBest; ++i) {
    if (OptimalWeightCmp(last[i].weight, min_value) == true) {
      min_value = last[i].weight;
      min_index = i;
    }
  }

  int previous_position,
      previous_index,
      position = token_instance->size(),
      index = min_index,
      term_type;
  std::string buffer;
  term_instance->set_size(decode_node_[position][index].term_position + 1);
  while (position != 0) {
    previous_position = decode_node_[position][index].previous_node_position;
    previous_index = decode_node_[position][index].previous_node_index;

    buffer.clear();
    for (int i = previous_position; i < position; ++i) {
      buffer.append(token_instance->token_text_at(i));
    }
    
    term_type = position - previous_position > 1? TermInstance::kChineseWord: 
                                                  token_type_to_word_type(token_instance->token_type_at(previous_position));
    term_instance->set_value_at(decode_node_[position][index].term_position,
                                buffer.c_str(),
                                position - previous_position,
                                term_type);
    // printf("---%d---\n", decode_node_[position][index].term_position);
    position = previous_position;
    index = previous_index;
  } // end while

  // Clear decode_node
  for (int i = 0; i < token_instance->size() + 1; ++i) {
    memset(decode_node_[i], 0, sizeof(OptimalNode) * kNBest);
    node_greatest_index[i] = 0;
  }

}







