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
#include <stdexcept>
#include "hmm_part_of_speech_tagger.h"
#include "part_of_speech_tag_instance.h"
#include "term_instance.h"
#include "configuration.h"
#include "utils.h"
#include "hmm_model.h"



struct HMMPartOfSpeechTagger::Node {
  int left_tag;
  double cost;
};



HMMPartOfSpeechTagger::HMMPartOfSpeechTagger(): model_(NULL),
                                                one_tag_emit_(NULL) {
  for (int i = 0; i < kMaxBucket; ++i) {
    buckets_[i] = NULL;
  }
}

HMMPartOfSpeechTagger::~HMMPartOfSpeechTagger() {
  for (int i = 0; i < kMaxBucket; ++i) {
    if (buckets_[i] != NULL) 
      delete[] buckets_[i];
    buckets_[i] = NULL;
  }

  if (one_tag_emit_ != NULL) {
    for (int i = 0; i < tag_num_; ++i) {
      delete one_tag_emit_[i];
      one_tag_emit_[i] = NULL;
    }
    delete[] one_tag_emit_;
  }

  delete model_;
  model_ = NULL;
}

HMMPartOfSpeechTagger *HMMPartOfSpeechTagger::Create(const char *model_path, 
                                                     const char *index_path,
                                                     const char *default_tag_path) {
  HMMPartOfSpeechTagger *self = new HMMPartOfSpeechTagger();
  
  Configuration *default_tag_conf = NULL;
  char error_message[1024];
  FILE *fd = NULL;

  try {
    self->model_ = HMMModel::Create(model_path);
    if (self->model_ == NULL) {
      throw std::runtime_error(std::string("load model file ") + model_path); 
    }

    self->tag_num_ = self->model_->tag_num();

    for (int i = 0; i < kMaxBucket; ++i) {
      self->buckets_[i] = new Node[self->tag_num_];
    }

    if (0 != self->unigram_trie_.open(index_path))
      throw std::runtime_error(std::string("unable to open index file  ") + index_path); 

    // Get default tags
    default_tag_conf = Configuration::LoadFromPath(default_tag_path);
    if (default_tag_conf == NULL) {
      throw std::runtime_error(std::string("unable to open default tag configuration file ") + default_tag_path); 
    }

    self->one_tag_emit_ = new HMMModel::EmitRow *[self->tag_num_];
    for (int i = 0; i < self->tag_num_; ++i) {
      self->one_tag_emit_[i] = new HMMModel::EmitRow();
      self->one_tag_emit_[i]->tag = i;
      self->one_tag_emit_[i]->cost = 20;
      self->one_tag_emit_[i]->next = NULL;
    }

    // Note that LoadDefaultTags will throw runtime_error
    self->LoadDefaultTags(default_tag_conf, "word", self->term_type_emit_tag_ + TermInstance::kChineseWord);
    self->LoadDefaultTags(default_tag_conf, "english", self->term_type_emit_tag_ + TermInstance::kEnglishWord);
    self->LoadDefaultTags(default_tag_conf, "number", self->term_type_emit_tag_ + TermInstance::kNumber);
    self->LoadDefaultTags(default_tag_conf, "symbol", self->term_type_emit_tag_ + TermInstance::kSymbol);
    self->LoadDefaultTags(default_tag_conf, "punction", self->term_type_emit_tag_ + TermInstance::kPunction);
    self->LoadDefaultTags(default_tag_conf, "other", self->term_type_emit_tag_ + TermInstance::kOther);

    delete default_tag_conf;
    return self;

  } catch (std::exception &ex) {
    set_error_message(ex.what());
    delete self;
    if (fd != NULL) fclose(fd);
    if (default_tag_conf != NULL) delete default_tag_conf;
    return NULL;
  }
}

int HMMPartOfSpeechTagger::GetTagIdByStr(const char *tag_str) {
  for (int i = 0; i < tag_num_; ++i) {
    if (strcmp(model_->GetTagStr(i), tag_str) == 0) {
      return i;
    }
  }

  return -1;
}

void HMMPartOfSpeechTagger::LoadDefaultTags(const Configuration *conf, const char *key, int *emit_tag) {

  if (conf->HasKey(key) == false) 
    throw std::runtime_error(std::string("unable to find key '") + key + "' in default tag configuration file");
  
  const char *tag_str = conf->GetString(key);
  int tag = GetTagIdByStr(tag_str);
  if (tag < 0) 
    throw std::runtime_error(std::string(tag_str) + " not exists in tag set");

  *emit_tag = tag;
}

void HMMPartOfSpeechTagger::BuildEmitTagfForNode(TermInstance *term_instance) {
  int term_id;
  HMMModel::EmitRow *emit_node;

  for (int i = 0; i < term_instance->size(); ++i) {
    term_id = term_instance->term_id_at(i);
    if (term_id == TermInstance::kTermIdNone) {
      term_id = unigram_trie_.exactMatchSearch<Darts::DoubleArray::value_type>(term_instance->term_text_at(i));
    }

    emit_node = model_->GetEmitRow(term_id);
    if (emit_node == NULL) {
      term_tags_[i] = one_tag_emit_[term_type_emit_tag_[term_instance->term_type_at(i)]];
      has_data_[i] = false; 
    } else {
      term_tags_[i] = emit_node;
      has_data_[i] = true;
    }
  }
}

void HMMPartOfSpeechTagger::Tag(PartOfSpeechTagInstance *part_of_speech_tag_instance, TermInstance *term_instance) {
  // Get each term's plausible emit tags
  BuildEmitTagfForNode(term_instance);

  // Clear the first node
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
  const HMMModel::EmitRow *emit_node = term_tags_[last_position];
  int min_tag;
  double min_cost = 1e37;
  while (emit_node) {
    if (last_bucket[emit_node->tag].cost < min_cost) {
      min_cost = last_bucket[emit_node->tag].cost;
      min_tag = emit_node->tag;
    }
    emit_node = emit_node->next;
  }

  int tag = min_tag;
  for (int position = last_position; position >= 0; --position) {
    part_of_speech_tag_instance->set_value_at(
        position, 
        model_->GetTagStr(tag), 
        term_instance->term_type_at(position) == TermInstance::kChineseWord && has_data_[position] == false);
    tag = buckets_[position][tag].left_tag;
  }

  part_of_speech_tag_instance->set_size(term_instance->size());
}

void HMMPartOfSpeechTagger::CalculateBucketCost(int position) {
  const HMMModel::EmitRow *emit_node = term_tags_[position];
  while (emit_node) {
    buckets_[position][emit_node->tag].cost += emit_node->cost;
    // printf("%s %lf\n", tag_str_[emit_node->tag_id], emit_node->weight);
    emit_node = emit_node->next;
  }
}

void HMMPartOfSpeechTagger::CalculateArcCost(int position) {
  const HMMModel::EmitRow *left_emit_node, *emit_node, *p;
  left_emit_node = term_tags_[position - 1];
  emit_node = term_tags_[position];

  double min_cost = 1e37,
         cost;
  int min_tag;
  while (emit_node) {
    p = left_emit_node;
    while (p) {
      cost = buckets_[position - 1][p->tag].cost + model_->GetTransCost(p->tag, emit_node->tag);
      // printf("%s %s %lf\n", tag_str_[p->tag_id], tag_str_[emit_node->tag_id], transition_matrix_[p->tag_id * tag_num_ + emit_node->tag_id]);
      if (cost < min_cost) {
        min_cost = cost;
        min_tag = p->tag;
      }
      p = p->next;
    }
    buckets_[position][emit_node->tag].left_tag = min_tag;
    buckets_[position][emit_node->tag].cost = min_cost;
    emit_node = emit_node->next;
  }
}