//
// bigram_anal.h --- Created at 2014-02-03
//
// The MIT License (MIT)
//
// Copyright 2014 ling0322 <ling032x@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef BIGRAM_ANAL_H
#define BIGRAM_ANAL_H

#include <vector>
#include <string>
#include <unordered_map>
#include "utils/status.h"

// Use bigram segmentation to analyze a corpus. Candidate to analyze is specified by 
// candidate, and the corpus is specified by corpus_path. It would use a temporary file
// called 'candidate_cost.txt' as user dictionary file for MilkCat.
// On success, stores the adjecent entropy in adjacent_entropy, and stores the vocabulary
// of segmentation's result in vocab. On failed set status != Status::OK()
void BigramAnalyze(const std::unordered_map<std::string, float> &candidate,
                   const char *corpus_path,
                   std::unordered_map<std::string, double> &adjacent_entropy,
                   std::unordered_map<std::string, int> &vocab,
                   Status &status);

#endif