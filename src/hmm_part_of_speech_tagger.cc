//
// hmm_part_of_speech_tagger.cc --- Created at 2013-11-10
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
#include <stdint.h>
#include "hmm_part_of_speech_tagger.h"
#include "part_of_speech_tag_instance.h"
#include "term_instance.h"
#include "configuration.h"
#include "utils.h"

struct HMMPartOfSpeechTagger::TermTagProbability {
  int tag_id;
  double weight;
  HMMPartOfSpeechTagger::TermTagProbability *next;
};

struct HMMPartOfSpeechTagger::Node {
  int left_tag;
  double weight;
};

#pragma pack(1)
struct HMMEmitRecord {
  int32_t term_id;
  int32_t tag_id;
  float weight;
};
#pragma pack(0)

HMMPartOfSpeechTagger::HMMPartOfSpeechTagger(): tag_str_(NULL),
                                                transition_matrix_(NULL),
                                                oth_emit_node_(NULL) {
  for (int i = 0; i < kMaxBucket; ++i) {
    buckets_[i] = NULL;
  }
}

HMMPartOfSpeechTagger::~HMMPartOfSpeechTagger() {
  if (transition_matrix_ != NULL) {
    delete[] transition_matrix_;
    transition_matrix_ = NULL;
  }

  if (tag_str_ != NULL) {
    delete[] tag_str_;
    tag_str_ = NULL;
  }

  TermTagProbability *p, *q;
  for (int i = 0; i < max_term_id_ + 1; ++i) {
    p = emit_matrix_[i];
    while (p) {
      q = p->next;
      delete p;
      p = q;
    } 
  }
  delete[] emit_matrix_;

  for (int i = 0; i < kMaxBucket; ++i) {
    if (buckets_[i] != NULL) 
      delete[] buckets_[i];
    buckets_[i] = NULL;
  }

  if (oth_emit_node_ != NULL) {
    delete oth_emit_node_;
    oth_emit_node_ = NULL;
  }
}

HMMPartOfSpeechTagger *HMMPartOfSpeechTagger::Create(const char *model_path, const char *index_path) {
  HMMPartOfSpeechTagger *self = new HMMPartOfSpeechTagger();
  char error_message[1024];

  FILE *fd = fopen(model_path, "r");
  if (fd == NULL) {
    sprintf(error_message, "unable to open HMM model file %s\n", model_path);
    goto create_failed;
  }

  // Read magic number
  int32_t magic_number;
  if (1 != fread(&magic_number, sizeof(int32_t), 1, fd)) {
    sprintf(error_message, "unable to read from file %s\n", model_path);
    goto create_failed;  
  }

  if (magic_number != 0x3322) {
    sprintf(error_message, "invalid HMM model file %s\n", model_path);
    goto create_failed;
  }

  int32_t tag_num;
  if (1 != fread(&tag_num, sizeof(int32_t), 1, fd)) {
    sprintf(error_message, "unable to read from file %s\n", model_path);
    goto create_failed;  
  }

  int32_t max_term_id;
  if (1 != fread(&max_term_id, sizeof(int32_t), 1, fd)) {
    sprintf(error_message, "unable to read from file %s\n", model_path);
    goto create_failed;  
  }

  int32_t emit_num;
  if (1 != fread(&emit_num, sizeof(int32_t), 1, fd)) {
    sprintf(error_message, "unable to read from file %s\n", model_path);
    goto create_failed;  
  }

  self->tag_str_ = reinterpret_cast<char (*)[16]>(new char[16 * tag_num]);
  for (int i = 0; i < tag_num; ++i) {
    if (1 != fread(self->tag_str_[i], 16, 1, fd)) {
      sprintf(error_message, "unable to read from file %s\n", model_path);
      goto create_failed;  
    }
  }

  self->transition_matrix_ = new double[tag_num * tag_num];
  float f_weight;
  for (int i = 0; i < tag_num * tag_num; ++i) {
    if (1 != fread(&f_weight, sizeof(float), 1, fd)) {
      sprintf(error_message, "unable to read from file %s\n", model_path);
      goto create_failed;  
    }
    self->transition_matrix_[i] = f_weight;
  }

  self->emit_matrix_ = new TermTagProbability *[max_term_id + 1];
  memset(self->emit_matrix_, 0, sizeof(TermTagProbability *) * (max_term_id + 1));
  HMMEmitRecord emit_record;
  TermTagProbability *emit_node;
  for (int i = 0; i < emit_num; ++i) {
    if (1 != fread(&emit_record, sizeof(emit_record), 1, fd)) {
      sprintf(error_message, "unable to read from file %s\n", model_path);
      goto create_failed;  
    }
    if (emit_record.term_id > max_term_id) {
      sprintf(error_message, "invalid HMM model file %s\n", model_path);
      goto create_failed;         
    }
    emit_node = new TermTagProbability();
    emit_node->tag_id = emit_record.tag_id;
    emit_node->weight = emit_record.weight;
    emit_node->next = self->emit_matrix_[emit_record.term_id];
    self->emit_matrix_[emit_record.term_id] = emit_node;
  }
  
  // Get a char to reach EOF
  fgetc(fd);
  if (0 == feof(fd)) {
    printf("%ld\n", ftell(fd));
    sprintf(error_message, "invalid HMM model file %s\n", model_path);
    goto create_failed;    
  }

  fclose(fd);
  self->tag_num_ = tag_num;
  self->max_term_id_ = max_term_id;

  for (int i = 0; i < kMaxBucket; ++i) {
    self->buckets_[i] = new Node[tag_num];
  }

  self->oth_emit_node_ = new TermTagProbability();
  self->oth_emit_node_->tag_id = kOthTag;
  self->oth_emit_node_->weight = 20;
  self->oth_emit_node_->next = NULL;

  if (0 != self->unigram_trie_.open(index_path)) {
    sprintf(error_message, "unable to open index file %s\n", index_path);
    goto create_failed;      
  }

  return self;

create_failed:
  set_error_message(error_message);
  delete self;
  if (fd != NULL) fclose(fd);
  return NULL;    
}

void HMMPartOfSpeechTagger::Tag(PartOfSpeechTagInstance *part_of_speech_tag_instance, TermInstance *term_instance) {
  for (int i = 0; i < term_instance->size(); ++i) {
    term_ids_[i] = unigram_trie_.exactMatchSearch<Darts::DoubleArray::value_type>(term_instance->term_text_at(i));
  }

  memset(buckets_[0], 0, tag_num_ * sizeof(Node));

  // Viterbi algorithm
  CalculateBucketCost(0);
  for (int i = 1; i < term_instance->size(); ++i) {
    CalculateArcCost(i);
    CalculateBucketCost(i);
  }

  // Find the best result
  int last_position = term_instance->size() - 1;
  const Node *last_bucket = buckets_[last_position];
  const TermTagProbability *emit_node = GetEmitLinkList(term_ids_[last_position]);
  int min_tag_id;
  double min_cost = 1e37;
  while (emit_node) {
    if (last_bucket[emit_node->tag_id].weight < min_cost) {
      min_cost = last_bucket[emit_node->tag_id].weight;
      min_tag_id = emit_node->tag_id;
    }
    emit_node = emit_node->next;
  }

  int tag_id = min_tag_id;
  for (int position = last_position; position >= 0; --position) {
    part_of_speech_tag_instance->set_value_at(position, tag_str_[tag_id]);
    tag_id = buckets_[position][tag_id].left_tag;
  }

  part_of_speech_tag_instance->set_size(term_instance->size());
}

const HMMPartOfSpeechTagger::TermTagProbability *HMMPartOfSpeechTagger::GetEmitLinkList(int term_id) const {
  if (term_id > max_term_id_ || term_id < 0) {
    return oth_emit_node_;
  } else {
    TermTagProbability *emit_node = emit_matrix_[term_id];
    if (emit_node == NULL) {
      return oth_emit_node_;
    } else {
      return emit_node;
    }
  }
}

void HMMPartOfSpeechTagger::CalculateBucketCost(int position) {
  const TermTagProbability *emit_node = GetEmitLinkList(term_ids_[position]);
  while (emit_node) {
    buckets_[position][emit_node->tag_id].weight += emit_node->weight;
    // printf("%s %lf\n", tag_str_[emit_node->tag_id], emit_node->weight);
    emit_node = emit_node->next;
  }
}

void HMMPartOfSpeechTagger::CalculateArcCost(int position) {
  const TermTagProbability *left_emit_node, *emit_node, *p;
  left_emit_node = GetEmitLinkList(term_ids_[position - 1]);
  emit_node = GetEmitLinkList(term_ids_[position]);

  double min_cost = 1e37,
         cost;
  int min_tag_id;
  while (emit_node) {
    p = left_emit_node;
    while (p) {
      cost = buckets_[position - 1][p->tag_id].weight + transition_matrix_[p->tag_id * tag_num_ + emit_node->tag_id];
      // printf("%s %s %lf\n", tag_str_[p->tag_id], tag_str_[emit_node->tag_id], transition_matrix_[p->tag_id * tag_num_ + emit_node->tag_id]);
      if (cost < min_cost) {
        min_cost = cost;
        min_tag_id = p->tag_id;
      }
      p = p->next;
    }
    buckets_[position][emit_node->tag_id].left_tag = min_tag_id;
    buckets_[position][emit_node->tag_id].weight = min_cost;
    emit_node = emit_node->next;
  }
}