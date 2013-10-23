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
#include "bigram_segmenter.h"
#include "milkcat_config.h"
#include "utils.h"

struct OptimalNode {
  // term_id for this node
  int term_id;

  // Weight summation in this path
  double weight;

  // Previous node position in decode_node_
  int previous_node_position;

  // Previous node index in the position of previous_node_position
  int previoud_node_index;
};

// Record struct for bigram data
#pragma pack(1)
struct BigramRecord {
  int32_t left_id;
  int32_t right_id;
  float weight;
};
#pragma pack(0)

BigramSegmenter::BigramSegmenter(int bigram_number): unigram_trie_(NULL),
                                                     unigram_weight_(NULL) {
  for (int i = 0; i < sizeof(decode_node_) / sizeof(OptimalNode *); ++i) {
    decode_node_ = NULL;
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
  for (int i = 0; i < sizeof(decode_node_) / sizeof(OptimalNode *); ++i) {
    decode_node_[i] = new OptimalNode[kNBest];
  }

  // Load unigram_trie file
  self->unigram_trie_ = new Darts::DoubleArray();
  if (-1 == self->unigram_trie_->open(trietree_path)) {
    delete self;
    sprintf(error_message, "unable to open index file %s", darts_path);
    set_error_message(error_message);
    return NULL;
  }

  // Load unigram weight file
  fd = fopen(unigram_binary_path, "r");
  if (fd == NULL) {
    delete self;
    sprintf(error_message, "unable to open unigram data file %s", unigram_binary_path);
    set_error_message(error_message);
    return NULL;    
  }

  fseek(fd, 0L, SEEK_END);
  file_size = ftell(fd);
  fseek(fd, 0L, SEEK_SET);
  record_number = file_size / sizeof(float);

  if (record_number != self->unigram_trie_->size()) {
    delete self;
    sprintf(error_message, "invalid record number, index_file = %d, data_file = %d", self->unigram_trie_->size(), file_size / sizeof(float));
    set_error_message(error_message);
    return NULL;     
  }

  self->unigram_weight_ = new float[record_number];
  if (record_number != fread(self->unigram_weight_, sizeof(float), self->unigram_trie_->size(), fd)) {
    delete self;
    sprintf(error_message, "unable to read from unigram data file", unigram_binary_path);
    set_error_message(error_message);
    return NULL;      
  }

  fclose(fd);

  // Load bigram weight file
  fd = fopen(bigram_binary_path, "r");
  if (fd == NULL) {
    delete self;
    sprintf(error_message, "unable to open bigram data file %s", bigram_binary_path);
    set_error_message(error_message);
    return NULL;    
  }

  fseek(fd, 0L, SEEK_END);
  file_size = ftell(fd);
  fseek(fd, 0L, SEEK_SET);
  record_number = file_size / sizeof(BigramRecord);

  // load_factor 0.5 for unordered map
  self->bigram_weight_.reserve(record_number * 2); 
  BigramRecord bigram_record;
  for (int i = 0; i < record_number; ++i) {
    if (1 != fread(&bigram_record, sizeof(bigram_record), 1, fd)) {
      delete self;
      sprintf(error_message, "unable to read from bigram data file %s", bigram_binary_path);
      set_error_message(error_message);
      return NULL;         
    }
    self->bigram_weight_[bigram_record.left_id << 32 + bigram_record.right_id] = bigram_record.weight;
  }

  fclose(fd);

  return self;
}

void BigramSegmenter::Process(TermInstance *term_instance, const TokenInstance *token_instance) {
  
}







