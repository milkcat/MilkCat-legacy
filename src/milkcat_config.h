#ifndef MILKCAT_CONFIG_H
#define MILKCAT_CONFIG_H

#include <stdlib.h>

const size_t kTokenMax = 1000;
const size_t kFeatureLengthMax = 100;
const size_t kTermLengthMax = kFeatureLengthMax;
const size_t kPOSTagLengthMax = 10;
const size_t kHMMSegmentAndPOSTaggingNBest = 3;

#endif