#ifndef CRF_TAGGER_H
#define CRF_TAGGER_H

#include "crfpp.h"
#include "tag_set.h"
#include "crfpp_tagger.h"

class Instance;
class TagSequence;
class FeatureExtractor;

class CRFTagger {
 public:
  ~CRFTagger();

  static CRFTagger *Create(const char *model_path, TagSet *tag_set);
  void Tag(FeatureExtractor *feature_extractor, TagSequence *tag_sequence) const {
  	TagRange(feature_extractor, tag_sequence, 0, feature_extractor->size());
  }
  
  void TagRange(FeatureExtractor *feature_extractor, TagSequence *tag_sequence, int begin, int end) const;

 private:
  CrfppTagger *crfpp_tagger_;
  CrfppModel *crfpp_model_;
  TagSet *tag_set_;
  TagSet::TagId *crfpp_tag_id_to_local_tag_id_;
  size_t *local_tag_id_to_crfpp_tag_id_;

  CRFTagger();
};


#endif