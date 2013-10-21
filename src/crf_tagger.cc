//
// YacaNLP -- Yet Another Chinese Analyzer 
// crf_tagger.cc -- Created at 2013-03-25
//
//
// Copyright (c) 2013 L. Chen <ling032x@gmail.com>
//


#include <stdio.h>
#include "crfpp.h"
#include "string.h"
#include "stdio.h"
#include "crf_tagger.h"
#include "tag_sequence.h"
#include "feature_extractor.h"
#include "tag_set.h"
#include "utils.h"


CRFTagger::CRFTagger(): crfpp_(NULL), 
                        crfpp_tag_id_to_local_tag_id_(NULL),
                        local_tag_id_to_crfpp_tag_id_(NULL) {
}

CRFTagger *CRFTagger::Create(const char *model_path, TagSet *tag_set) {
  char crf_param[4096];

  CRFTagger *self = new CRFTagger();
  self->tag_set_ = tag_set;
  self->crfpp_tag_id_to_local_tag_id_ = new TagSet::TagId[self->tag_set_->size()];
  self->local_tag_id_to_crfpp_tag_id_ = new size_t[self->tag_set_->size()];

  sprintf(crf_param, "-m %s", model_path);
  self->crfpp_ = CRFPP::createTagger(crf_param);
  if (self->crfpp_ == NULL) {
    delete self;
    return NULL;
  }

  //
  // init the map of crfpp's internal tag-id to Tagger's tag-id
  //
  for (size_t i = 0; i < self->crfpp_->ysize(); ++i) {
    self->crfpp_tag_id_to_local_tag_id_[i] = self->tag_set_->TagStringToTagId(self->crfpp_->yname(i));
  };

  return self;
}

CRFTagger::~CRFTagger() {
  if (crfpp_ != NULL) {
    delete crfpp_;
    crfpp_ = NULL;    
  }

  if (crfpp_tag_id_to_local_tag_id_ != NULL) {
    delete crfpp_tag_id_to_local_tag_id_;
    crfpp_tag_id_to_local_tag_id_ = NULL;
  }

  if (local_tag_id_to_crfpp_tag_id_ != NULL) {
    delete local_tag_id_to_crfpp_tag_id_;
    local_tag_id_to_crfpp_tag_id_ = NULL;
  }
}

void CRFTagger::Tag(FeatureExtractor *feature_extractor, TagSequence *tag_sequence) const {
  size_t size = feature_extractor->size();

  crfpp_->clear();
  tag_sequence->set_length(size);
  const char **features;

  for (size_t i = 0 ; i < size; ++i) {
    features = feature_extractor->ExtractFeatureAt(i);
    crfpp_->add(feature_extractor->feature_number(), features);
    // for (int j = 0; j < feature_extractor->feature_number(); ++j) { printf("%s ", features[j]); }
  }
  // printf("]");

  crfpp_->parse();
  for (size_t i = 0; i < size; ++i) {
    // printf("%d\n", i);
    tag_sequence->SetTagAt(i, crfpp_tag_id_to_local_tag_id_[crfpp_->y(i)]);
    //cprintf("%s  ", tag_set_->TagIdToTagString(crfpp_tag_id_to_local_tag_id_[crfpp_->y(i)]));
  }
  // printf("\n");
}
