/*
 * hmm_segment_and_pos_tagger.h
 *
 * by ling0322 at 2013-08-14
 *
 */


#ifndef HMM_SEGMENTER_POS_TAGGER_H
#define HMM_SEGMENTER_POS_TAGGER_H

#include <algorithm>
#include <vector>
#include <assert.h>
#include <string.h>
#include "darts.h"
#include "milkcat_config.h"
#include "part_of_speech_tag_instance.h"
#include "tag_set.h"
#include "utils.h"

class POSTagSet;

class HMMSegmentAndPOSTagger {
 public:
  
  static HMMSegmentAndPOSTagger *Create(const char *darts_path, const char *weight_file_path, const char *trans_file_path);
  ~HMMSegmentAndPOSTagger();
  void Process(TermInstance *term_instance, 
               PartOfSpeechTagInstance *part_of_speech_tag_instance, 
               const TokenInstance *token_instance);

 private:
  class Node;
  struct POSTagWeight {
    TagSet::TagId POS_tag;
    double weight;
    POSTagWeight *next;
  };

  Node *decode_node_;
  Darts::DoubleArray *double_array_;
  POSTagWeight **weight_list_;
  double **trans_matrix_;
  POSTagSet *pos_tag_set_;
  size_t term_number_;

  HMMSegmentAndPOSTagger(): decode_node_(NULL),
                            double_array_(NULL),
                            weight_list_(NULL),
                            trans_matrix_(NULL),
                            pos_tag_set_(NULL) {
  }

  void Viterbi(const TokenInstance *token_instance);
  inline void ViterbiAddOtherCharPath(int node_id, TagSet::TagId tag_id);
  void FindBestResult(TermInstance *term_instance, 
                      PartOfSpeechTagInstance *part_of_speech_tag_instance, 
                      const TokenInstance *token_instance);
};

class HMMSegmentAndPOSTagger::Node {
 public:
  Node() {}

  class Block {
   public:
    Block(TagSet::TagId POS_tag, int previous_node_id, int previous_block_id, double weight):
        previous_node_id(previous_node_id),
        previous_block_id(previous_block_id),
        POS_tag(POS_tag),
        weight(weight) {
    }

    Block() {}

    TagSet::TagId POS_tag;
    int previous_node_id;
    int previous_block_id;
    double weight; 
  };

  // Add a block into the node, if no more space then replace the 
  // node of highest-weight
  void AddBlock(
      TagSet::TagId POS_tag, 
      int previous_node_id, 
      int previous_block_id, 
      double weight);

  double highest_weight() { return highest_weight_; }
  int highest_weight_node_id() { return highest_weight_node_id_; }
  int block_num() { return block_num_; }
  const Block *GetBlock(int index) { assert(index < block_num_); return &block_[index]; }
  void Clear() {
    block_num_ = 0;
    highest_weight_ = 1000000000.0;
    highest_weight_node_id_ = -1;
  }

 private:
  Block block_[kHMMSegmentAndPOSTaggingNBest];
  int block_num_;
  double highest_weight_;
  int highest_weight_node_id_;

  DISALLOW_COPY_AND_ASSIGN(Node);
};


#endif