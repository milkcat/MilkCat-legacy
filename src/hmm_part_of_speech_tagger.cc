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
                                                one_tag_emit_(NULL) {
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

  if (one_tag_emit_ != NULL) {
    for (int i = 0; i < tag_num_; ++i) {
      if (one_tag_emit_[i] != NULL) {
        delete one_tag_emit_[i];
        one_tag_emit_[i] = NULL;
      }
    }
    delete[] one_tag_emit_;
  }
}

HMMPartOfSpeechTagger *HMMPartOfSpeechTagger::Create(const char *model_path, 
                                                     const char *index_path,
                                                     const char *default_tag_path) {
  HMMPartOfSpeechTagger *self = new HMMPartOfSpeechTagger();
  Configuration *default_tag_conf = NULL;
  char error_message[1024];
  FILE *fd = NULL;

  try {
    fd = fopen(model_path, "r");
    if (fd == NULL) 
      throw std::runtime_error(std::string("unable to open HMM model file ") + model_path);

    // Read magic number
    int32_t magic_number;
    if (1 != fread(&magic_number, sizeof(int32_t), 1, fd)) 
      throw std::runtime_error(std::string("unable to read from file ") + model_path);

    if (magic_number != 0x3322)
      throw std::runtime_error(std::string("invalid HMM model file ") + model_path);

    int32_t tag_num;
    if (1 != fread(&tag_num, sizeof(int32_t), 1, fd))
      throw std::runtime_error(std::string("unable to read from file ") + model_path);

    int32_t max_term_id;
    if (1 != fread(&max_term_id, sizeof(int32_t), 1, fd))
      throw std::runtime_error(std::string("unable to read from file ") + model_path);

    int32_t emit_num;
    if (1 != fread(&emit_num, sizeof(int32_t), 1, fd))
      throw std::runtime_error(std::string("unable to read from file ") + model_path);

    self->tag_str_ = reinterpret_cast<char (*)[16]>(new char[16 * tag_num]);
    for (int i = 0; i < tag_num; ++i) {
      if (1 != fread(self->tag_str_[i], 16, 1, fd)) 
        throw std::runtime_error(std::string("unable to read from file ") + model_path);
    }

    self->transition_matrix_ = new double[tag_num * tag_num];
    float f_weight;
    for (int i = 0; i < tag_num * tag_num; ++i) {
      if (1 != fread(&f_weight, sizeof(float), 1, fd))
        throw std::runtime_error(std::string("unable to read from file ") + model_path);
      self->transition_matrix_[i] = f_weight;
    }


    self->emit_matrix_ = new TermTagProbability *[max_term_id + 1];
    memset(self->emit_matrix_, 0, sizeof(TermTagProbability *) * (max_term_id + 1));
    HMMEmitRecord emit_record;
    TermTagProbability *emit_node;
    for (int i = 0; i < emit_num; ++i) {
      if (1 != fread(&emit_record, sizeof(emit_record), 1, fd)) 
        throw std::runtime_error(std::string("unable to read from file ") + model_path);
      if (emit_record.term_id > max_term_id) 
        throw std::runtime_error(std::string("unable to read from file ") + model_path);
      emit_node = new TermTagProbability();
      emit_node->tag_id = emit_record.tag_id;
      emit_node->weight = emit_record.weight;
      emit_node->next = self->emit_matrix_[emit_record.term_id];
      self->emit_matrix_[emit_record.term_id] = emit_node;
    }

    // Get a char to reach EOF
    fgetc(fd);
    if (0 == feof(fd))
      throw std::runtime_error(std::string("invalid HMM model file ") + model_path); 

    fclose(fd);
    fd = NULL;
    self->tag_num_ = tag_num;
    self->max_term_id_ = max_term_id;

    for (int i = 0; i < kMaxBucket; ++i) {
      self->buckets_[i] = new Node[tag_num];
    }

    if (0 != self->unigram_trie_.open(index_path))
      throw std::runtime_error(std::string("unable to open index file  ") + index_path); 

    // Get default tags
    default_tag_conf = Configuration::LoadFromPath(default_tag_path);
    if (default_tag_conf == NULL) {
      throw std::runtime_error(std::string("unable to open default tag configuration file ") + default_tag_path); 
    }

    self->one_tag_emit_ = new TermTagProbability *[self->tag_num_];
    for (int i = 0; i < self->tag_num_; ++i) {
      self->one_tag_emit_[i] = new TermTagProbability();
      self->one_tag_emit_[i]->tag_id = i;
      self->one_tag_emit_[i]->weight = 20;
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
    if (strcmp(tag_str_[i], tag_str) == 0) {
      return i;
    }
  }

  return -1;
}

void HMMPartOfSpeechTagger::LoadDefaultTags(const Configuration *conf, const char *key, int *emit_tag) {

  if (conf->HasKey(key) == false) 
    throw std::runtime_error(std::string("unable to find key '") + key + "' in default tag configuration file");
  
  const char *tag = conf->GetString(key);
  int tag_id = GetTagIdByStr(tag);
  if (tag_id < 0) 
    throw std::runtime_error(std::string(tag) + " not exists in tag set");

  *emit_tag = tag_id;
}

void HMMPartOfSpeechTagger::BuildEmitTagfForNode(TermInstance *term_instance) {
  int term_id;
  TermTagProbability *emit_node;

  for (int i = 0; i < term_instance->size(); ++i) {
    term_id = term_instance->term_id_at(i);
    if (term_id == TermInstance::kTermIdNone) {
      term_id = unigram_trie_.exactMatchSearch<Darts::DoubleArray::value_type>(term_instance->term_text_at(i));
    }
    
    if (term_id > max_term_id_ || term_id < 0) {
      term_tags_[i] = one_tag_emit_[term_type_emit_tag_[term_instance->term_type_at(i)]];
      has_data_[i] = false;
    } else {
      emit_node = emit_matrix_[term_id];
      if (emit_node == NULL) {
        term_tags_[i] = one_tag_emit_[term_type_emit_tag_[term_instance->term_type_at(i)]];
        has_data_[i] = false; 
      } else {
        term_tags_[i] = emit_node;
        has_data_[i] = true;
      }
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
  const TermTagProbability *emit_node = term_tags_[last_position];
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
    part_of_speech_tag_instance->set_value_at(
        position, 
        tag_str_[tag_id], 
        term_instance->term_type_at(position) == TermInstance::kChineseWord && has_data_[position] == false);
    tag_id = buckets_[position][tag_id].left_tag;
  }

  part_of_speech_tag_instance->set_size(term_instance->size());
}

void HMMPartOfSpeechTagger::CalculateBucketCost(int position) {
  const TermTagProbability *emit_node = term_tags_[position];
  while (emit_node) {
    buckets_[position][emit_node->tag_id].weight += emit_node->weight;
    // printf("%s %lf\n", tag_str_[emit_node->tag_id], emit_node->weight);
    emit_node = emit_node->next;
  }
}

void HMMPartOfSpeechTagger::CalculateArcCost(int position) {
  const TermTagProbability *left_emit_node, *emit_node, *p;
  left_emit_node = term_tags_[position - 1];
  emit_node = term_tags_[position];

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