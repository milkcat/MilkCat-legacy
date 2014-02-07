//
// neko_main.cc --- Created at 2014-02-03
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

#include <stdio.h>
#include <unordered_map>
#include <vector>
#include <string>
#include "neko/candidate.h"
#include "neko/crf_vocab.h"
#include "neko/bigram_anal.h"
#include "neko/mutual_information.h"
#include "neko/final_rank.h"
#include "utils/writable_file.h"

int main(int argc, char **argv) {
  Status status;
  int total_count = 0;
  std::unordered_map<std::string, int> vocab = GetCrfVocabulary(argv[1], total_count, status);
  std::unordered_map<std::string, float> candidates;
  if (status.ok()) {
    candidates = GetCandidate("model.txt", "unigram.txt", vocab, total_count, 8, status);
  }
  
  WritableFile *fd = nullptr;
  if (status.ok()) fd = WritableFile::New("candidate_cost.txt", status);

  char line[1024];
  if (status.ok()) {
    for (auto &x: candidates) {
      sprintf(line, "%s %.5f", x.first.c_str(), x.second);
      fd->WriteLine(line, status);
    }
  }

  delete fd;

  vocab.clear();
  std::unordered_map<std::string, double> adjacent_entropy;
  std::unordered_map<std::string, double> mutual_information;
  
  if (status.ok()) BigramAnalyze(candidates, argv[1], adjacent_entropy, vocab, status);
  if (status.ok()) mutual_information = GetMutualInformation(vocab, candidates, status);

  if (status.ok()) {
    auto final_rank = FinalRank(adjacent_entropy, mutual_information);
    for (auto &x: final_rank) {
      printf("%s %f\n", x.first.c_str(), x.second);
    } 
  }

  if (!status.ok()) {
    printf("%s\n", status.what());
  }

  return 0;
}