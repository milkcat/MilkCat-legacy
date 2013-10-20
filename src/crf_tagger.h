#ifndef CRF_TAGGER_H
#define CRF_TAGGER_H

#include "crfpp.h"
#include "tag_set.h"

class Instance;
class TagSequence;
class FeatureExtractor;

class CRFTagger {
 public:
  ~CRFTagger();

  static CRFTagger *Create(const char *model_path, TagSet *tag_set);
  void Tag(FeatureExtractor *feature_extractor, TagSequence *tag_sequence) const;

 private:
  CRFPP::Tagger *crfpp_;
  TagSet *tag_set_;
  TagSet::TagId *crfpp_tag_id_to_local_tag_id_;
  size_t *local_tag_id_to_crfpp_tag_id_;

  CRFTagger();
};


#endif