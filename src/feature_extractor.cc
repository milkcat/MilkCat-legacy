/*
 * feature_extractor.cc
 *
 * by ling0322 at 2013-10-09
 *
 */

#include "feature_extractor.h"
#include "milkcat_config.h"

FeatureExtractor::FeatureExtractor(size_t feature_number): feature_number_(feature_number) {
  feature_list_ = new char *[feature_number];
  for (size_t i = 0; i < feature_number; ++i) {
    feature_list_[i] = new char[kFeatureLengthMax];
  }
}

FeatureExtractor::~FeatureExtractor() {
  for (size_t i = 0; i < feature_number_; ++i) {
    delete feature_list_[i];
  }
  delete feature_list_;
}