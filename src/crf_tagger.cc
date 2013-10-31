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


CRFTagger::CRFTagger(): crfpp_model_(NULL), 
                        crfpp_tagger_(NULL),
                        crfpp_tag_id_to_local_tag_id_(NULL),
                        local_tag_id_to_crfpp_tag_id_(NULL) {
}

CRFTagger *CRFTagger::Create(const char *model_path, TagSet *tag_set) {
  CRFTagger *self = new CRFTagger();
  self->tag_set_ = tag_set;
  self->crfpp_tag_id_to_local_tag_id_ = new TagSet::TagId[self->tag_set_->size()];
  self->local_tag_id_to_crfpp_tag_id_ = new size_t[self->tag_set_->size()];

  self->crfpp_model_ = CrfppModel::Create(model_path);
  if (self->crfpp_model_ == NULL) {
    delete self;
    return NULL;
  }
  self->crfpp_tagger_ = new CrfppTagger(self->crfpp_model_);

  //
  // init the map of crfpp's internal tag-id to Tagger's tag-id
  //
  for (size_t i = 0; i < self->crfpp_model_->GetTagNumber(); ++i) {
    self->crfpp_tag_id_to_local_tag_id_[i] = self->tag_set_->TagStringToTagId(self->crfpp_model_->GetTagText(i));
  };

  return self;
}

CRFTagger::~CRFTagger() {
  if (crfpp_model_ != NULL) {
    delete crfpp_model_;
    crfpp_model_ = NULL;    
  }

  if (crfpp_tagger_ != NULL) {
    delete crfpp_tagger_;
    crfpp_tagger_ = NULL;    
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

void CRFTagger::TagRange(FeatureExtractor *feature_extractor, TagSequence *tag_sequence, int begin, int end) const {
  int size = end - begin;

  tag_sequence->set_length(size);
  crfpp_tagger_->TagRange(feature_extractor, begin, end);
  for (size_t i = 0; i < size; ++i) {
    // printf("%d\n", i);
    tag_sequence->SetTagAt(i, crfpp_tag_id_to_local_tag_id_[crfpp_tagger_->GetTagAt(i)]);
    // printf("%s  ", tag_set_->TagIdToTagString(crfpp_tag_id_to_local_tag_id_[crfpp_tagger_->GetTagAt(i)]));
  }
  // printf("\n");
}
