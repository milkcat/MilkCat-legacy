/*
 * hmm_segment_and_pos_tagger.cc
 *
 * by ling0322 at 2013-08-15
 *
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>
#include "darts.h"
#include "utils.h"
#include "hmm_segment_and_pos_tagger.h"
#include "instance.h"
#include "milkcat_config.h"
#include "pos_tag_set.h"
#include "token_instance.h"
#include "term_instance.h"


HMMSegmentAndPOSTagger *HMMSegmentAndPOSTagger::Create(const char *darts_path, 
                                                       const char *weight_file_path, 
                                                       const char *trans_file_path) {
  char line_buf[1024];
  char term[kTermLengthMax];
  char POS_tag[kPOSTagLengthMax];
  char POS_tag_next[kPOSTagLengthMax];
  double weight;
  char error_message[2048];
  HMMSegmentAndPOSTagger *self = new HMMSegmentAndPOSTagger();

  self->pos_tag_set_ = new POSTagSet();
  self->decode_node_ = new HMMSegmentAndPOSTagger::Node[kTokenMax];
  
  self->double_array_ = new Darts::DoubleArray();
  if (-1 == self->double_array_->open(darts_path)) {
    delete self;
    sprintf(error_message, "unable to open word double array file %s", darts_path);
    set_error_message(error_message);
    return NULL;
  }

  self->term_number_ = self->double_array_->nonzero_size();
  self->weight_list_ = new POSTagWeight *[self->term_number_];
  memset(self->weight_list_, 0, sizeof(POSTagWeight *) * self->term_number_);

  // load term-tag weight data
  FILE *fp = fopen(weight_file_path, "r");
  if (fp == NULL) {
    delete self;
    sprintf(error_message, "unable to open term-tag weight file %s", weight_file_path);
    set_error_message(error_message);
    return NULL;
  }
  
  char *p_space1, *p_space2;
  while (fgets(line_buf, 1024, fp)) {
    
    p_space1 = strchr(line_buf, ' ');
    strncpy(term, line_buf, p_space1 - line_buf);
    term[p_space1 - line_buf] = 0;
    p_space2 = strrchr(p_space1, ' ');
    strncpy(POS_tag, p_space1 + 1, p_space2 - p_space1 - 1);
    POS_tag[p_space2 - p_space1 - 1] = 0;
    weight = atof(p_space2 + 1);
    // printf("%d %s %lf || %s\n", strlen(term), POS_tag, weight, line_buf);
    int term_id = self->double_array_->exactMatchSearch<int>(term);
    // printf("%s %s %d\n", term, POS_tag, term_id);
    assert(term_id >= 0);
    // puts(line_buf);
    TagSet::TagId tag_id = self->pos_tag_set_->TagStringToTagId(POS_tag);

    // create node
    POSTagWeight *link = new POSTagWeight();
    link->POS_tag = tag_id;
    link->weight = weight;
    link->next = self->weight_list_[term_id];
    self->weight_list_[term_id] = link;
  }
  fclose(fp);

  // load trans data
  fp = fopen(trans_file_path, "r");
  if (fp == NULL) {
    delete self;
    sprintf(error_message, "unable to open tag trans file %s", trans_file_path);
    set_error_message(error_message);
    return NULL;
  }
  self->trans_matrix_ = new double *[self->pos_tag_set_->size() + 2];  // p
  for (int i = 0; i < self->pos_tag_set_->size(); ++i) 
    self->trans_matrix_[i] = new double[self->pos_tag_set_->size()];

  TagSet::TagId tag_id;
  TagSet::TagId tag_id_next;
  while (fgets(line_buf, 1024, fp)) {
    sscanf(line_buf, "%s %s %lf", POS_tag, POS_tag_next, &weight);  
    tag_id = self->pos_tag_set_->TagStringToTagId(POS_tag);
    tag_id_next = self->pos_tag_set_->TagStringToTagId(POS_tag_next);
    self->trans_matrix_[tag_id][tag_id_next] = weight;
  }
  fclose(fp);

  return self;
}

HMMSegmentAndPOSTagger::~HMMSegmentAndPOSTagger() {
  if (double_array_ != NULL) {
    delete double_array_;
    double_array_ = NULL;
  }

  if (weight_list_ != NULL) {
    for (size_t term_id = 0; term_id < term_number_; ++term_id) {
      HMMSegmentAndPOSTagger::POSTagWeight *p = weight_list_[term_id], *q;
      while (p) {
        q = p->next;
        delete p;
        p = q;
      }
    }
    delete[] weight_list_;
  }

  if (trans_matrix_ != NULL) {
    for (TagSet::TagId tag_id = 0; tag_id < pos_tag_set_->size(); ++tag_id) {
      delete[] trans_matrix_[tag_id];
    }
    delete[] trans_matrix_;
  }

  if (pos_tag_set_ != NULL) {
    delete pos_tag_set_;
    pos_tag_set_ = NULL;
  }

  if (decode_node_ != NULL) {
    delete decode_node_;
    decode_node_ = NULL;
  }
}

void HMMSegmentAndPOSTagger::Node::AddBlock(
    TagSet::TagId POS_tag, 
    int previous_node_id,
    int previous_block_id,
    double weight) {

  // assert(weight < highest_weight_);

  if (block_num_ < kHMMSegmentAndPOSTaggingNBest) {
    block_[block_num_] = Block(POS_tag, previous_node_id, previous_block_id, weight);
    block_num_++;
  } else {
    block_[highest_weight_node_id_] = Block(POS_tag, previous_node_id, previous_block_id, weight);
  }
  
  // recalculate the highest weight node 
  highest_weight_ = 0;
  highest_weight_node_id_ = 0;
  for (int i = 0; i < block_num_; ++i) {
    if (block_[i].weight > highest_weight_) {
      highest_weight_ = block_[i].weight;
      highest_weight_node_id_ = i;
    }
  }
}

inline void HMMSegmentAndPOSTagger::ViterbiAddOtherCharPath(int node_id, TagSet::TagId tag_id) {
  const HMMSegmentAndPOSTagger::Node::Block *block;
  double weight;
  
  for (int block_id = 0; block_id < decode_node_[node_id].block_num(); ++block_id) {
    block = decode_node_[node_id].GetBlock(block_id);
    weight = block->weight + trans_matrix_[block->POS_tag][tag_id] + 20.0;
    decode_node_[node_id + 1].AddBlock(tag_id, node_id, block_id, weight);
  }
}

void HMMSegmentAndPOSTagger::Viterbi(const TokenInstance *token_instance) {
  double weight;
  size_t node_pos = 0;
  TagSet::TagId tag_NN = pos_tag_set_->TagStringToTagId("NN");
  TagSet::TagId tag_PU = pos_tag_set_->TagStringToTagId("PU");
  TagSet::TagId tag_CD = pos_tag_set_->TagStringToTagId("CD");


  for (size_t node_id = 0; node_id < token_instance->size() + 1; ++node_id)
    decode_node_[node_id].Clear();

  // start Node
  decode_node_[0].AddBlock(pos_tag_set_->TagStringToTagId("PU"), -1, -1, 0);

  // viterbi decoder
  for (size_t i = 0; i < token_instance->size(); ++i) {

    // not a chinese character
    switch (token_instance->integer_at(i, TokenInstance::kTokenTypeI)) {
     case TokenInstance::kPeriod:
      ViterbiAddOtherCharPath(i, tag_PU);
      continue;

     case TokenInstance::kNumber:
      ViterbiAddOtherCharPath(i, tag_CD);
      continue;

     case TokenInstance::kEnglishWord:
      ViterbiAddOtherCharPath(i, tag_NN);
      continue;

     case TokenInstance::kPunctuation:
      ViterbiAddOtherCharPath(i, tag_PU);
      continue;

     case TokenInstance::kOther:
      ViterbiAddOtherCharPath(i, tag_PU);
      continue;
    }

    node_pos = 0;
    for (int j = 0; i + j < token_instance->size(); ++j) {
      size_t key_pos = 0;

      Darts::DoubleArray::value_type term_id = double_array_->traverse(token_instance->string_at(i + j, TokenInstance::kTokenUTF8S),
                                                                       node_pos,
                                                                       key_pos);
      if (term_id >= 0) {
        for (int block_id = 0; block_id < decode_node_[i].block_num(); ++block_id) {
          for (POSTagWeight *weight_node = weight_list_[term_id]; 
               weight_node != NULL; 
               weight_node = weight_node->next) {
            
            const HMMSegmentAndPOSTagger::Node::Block *b = decode_node_[i].GetBlock(block_id);
            
            weight = b->weight + weight_node->weight + trans_matrix_[b->POS_tag][weight_node->POS_tag];
            // printf("%d %d %s %lf\n", i, j, pos_tag_set_->TagIdToTagString(weight_node->POS_tag), weight);
            if (weight < decode_node_[i + j + 1].highest_weight() || decode_node_[i + j + 1].block_num() < kHMMSegmentAndPOSTaggingNBest) {
              decode_node_[i + j + 1].AddBlock(weight_node->POS_tag, i, block_id, weight);
            }
          }          
        }
      } else {


        // the token not exists in dictionary
        if (j == 0 && decode_node_[i + j + 1].block_num() == 0) {
          // printf("%d OOV\n", i);
          ViterbiAddOtherCharPath(i, tag_PU);
        }

        if (term_id == -2) break;
      }
    }
  }  
}


void HMMSegmentAndPOSTagger::FindBestResult(TermInstance *term_instance, 
                                            PartOfSpeechTagInstance *part_of_speech_tag_instance, 
                                            const TokenInstance *token_instance) {
  std::vector<std::string> terms;
  std::vector<int> term_types;
  std::vector<TagSet::TagId> POS_tags;
  std::vector<int> term_token_number;
  std::string buffer;

  // Find the the last node of the best result
  int last_node = token_instance->size();
  int lowest_block_id = 0;
  double lowest_weight = 100000000.0;
  for (int block_id = 0; block_id < decode_node_[last_node].block_num(); ++block_id) {
    const HMMSegmentAndPOSTagger::Node::Block *block = decode_node_[last_node].GetBlock(block_id);
    if (block->weight < lowest_weight) {
      lowest_block_id = block_id;
      lowest_weight = block->weight;
    }
  }

  // Traverse the best result's node chain, and put the value of each node into some vectors
  int current_node = last_node;
  const HMMSegmentAndPOSTagger::Node::Block *current_block = decode_node_[current_node].GetBlock(lowest_block_id);
  int previous_node;
  while (current_node != 0) {
    buffer.clear();
    previous_node = current_block->previous_node_id;
    for (int i = previous_node; i < current_node; ++i) {
      buffer.append(token_instance->string_at(i, TokenInstance::kTokenUTF8S));
    }
    terms.push_back(buffer);

    // set the term type
    int token_number = current_node - previous_node;
    if (token_number > 1) {
      term_types.push_back(TermInstance::kChineseWord);
    } else {
      term_types.push_back(token_type_to_word_type(token_instance->integer_at(previous_node, TokenInstance::kTokenTypeI)));
    }
    
    POS_tags.push_back(current_block->POS_tag);
    term_token_number.push_back(token_number);

    current_block = decode_node_[previous_node].GetBlock(current_block->previous_block_id);
    current_node = previous_node;
  }

  int length = terms.size();
  for (int i = 0; i < length; ++i) {
    term_instance->set_value_at(length - i - 1, terms[i].c_str(), term_token_number[i], term_types[i]);
    part_of_speech_tag_instance->set_value_at(length - i - 1, pos_tag_set_->TagIdToTagString(POS_tags[i]));
  }

  term_instance->set_size(length);
  part_of_speech_tag_instance->set_size(length);
}

void HMMSegmentAndPOSTagger::Process(TermInstance *term_instance, 
                                     PartOfSpeechTagInstance *part_of_speech_tag_instance, 
                                     const TokenInstance *token_instance) {
  Viterbi(token_instance);
  FindBestResult(term_instance, part_of_speech_tag_instance, token_instance);
}