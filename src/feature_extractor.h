/*
 * feature_extractor.h
 *
 * by ling0322 at 2013-10-09
 *
 */

#ifndef FEATURE_EXTRACTOR_H
#define FEATURE_EXTRACTOR_H

#include <stdlib.h>
#include "milkcat_config.h"

class FeatureExtractor {
 public:
  virtual void ExtractFeatureAt(size_t position, char (*feature_list)[kFeatureLengthMax], int list_size) = 0;
  virtual size_t size() const = 0;
};

#endif