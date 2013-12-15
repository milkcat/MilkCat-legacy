#ifndef MILKCAT_CONFIG_H
#define MILKCAT_CONFIG_H

#include <stdlib.h>

const int kTokenMax = 1000;
const int kFeatureLengthMax = 100;
const int kTermLengthMax = kFeatureLengthMax;
const int kPOSTagLengthMax = 10;
const int kHMMSegmentAndPOSTaggingNBest = 3;
const int kUserTermId = 0x10000000;

#endif