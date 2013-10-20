/*
 * feature_extractor.h
 *
 * by ling0322 at 2013-10-09
 *
 */

#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include <stdlib.h>

class FeatureExtractor {
 public:
  FeatureExtractor(size_t feature_number);
  virtual ~FeatureExtractor();
  virtual const char **ExtractFeatureAt(size_t position) = 0;
  virtual size_t size() const = 0;
  size_t feature_number() { return feature_number_; }

 protected:
  char **feature_list_;
  size_t feature_number_;
};

#endif