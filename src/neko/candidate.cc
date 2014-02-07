//
// candidate.cc --- Created at 2014-02-02
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
#include <string.h>
#include <math.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "utils/readable_file.h"
#include "maxent_classifier.h"
#include "crf_vocab.h"
#include "utf8.h"

// Extracts features from name_str for the maxent classifier
std::vector<std::string> ExtractNameFeature(const char *name_str) {
  std::vector<std::string> feature_list;
  std::vector<std::string> utf8_chars;

  // Split the name_str into a vector of utf8 characters
  const char *it = name_str;
  char utf8_char[16], *end;
  while (*it) {
    uint32_t cp = utf8::next(it);
    end = utf8::append(cp, utf8_char);
    *end = '\0';
    utf8_chars.push_back(utf8_char);
  }

  if (utf8_chars.size() > 0) {
    feature_list.push_back(std::string("B:") + utf8_chars[0]);
    feature_list.push_back(std::string("E:") + utf8_chars[utf8_chars.size() - 1]);
    for (auto it = utf8_chars.begin() + 1; it < utf8_chars.end() - 1; ++it) {
      feature_list.push_back(std::string("M:") + *it);
    }
  }
  
  return feature_list;
}

// Load vocabulary (word, frequency) from the file specified by path. On success, return 
// the vocabulary. On failed, set status != Status::OK()
std::unordered_map<std::string, int> LoadVocabulary(const char *path, Status &status) {
  ReadableFile *fd = ReadableFile::New(path, status);
  std::unordered_map<std::string, int> vocab;
  char line[1024], word[1024];
  int count;

  while (status.ok() && !fd->Eof()) {
    fd->ReadLine(line, 1024, status);
    if (status.ok()) {
      sscanf(line, "%s %d", word, &count);
      vocab[word] = count;
    }
  }

  delete fd;
  return vocab;
}

// Get the candidate from crf segmentation vocabulary specified by crf_vocab.
// Returns a map the key is the word, and the value is its cost in unigram, which is
// used for bigram segmentation
std::unordered_map<std::string, float> GetCandidate(const char *model_path,
                                                    const char *dict_path,
                                                    const std::unordered_map<std::string, int> &crf_vocab, 
                                                    int total_count,
                                                    int thres_freq,
                                                    Status &status) {
  std::unordered_map<std::string, float> candidates;
  auto original_vocab = LoadVocabulary(dict_path, status);

  MaxentModel *name_model = nullptr;
  if (status.ok()) name_model = MaxentModel::New(model_path, status);

  MaxentClassifier *classifier = nullptr;
  if (status.ok()) classifier = new MaxentClassifier(name_model);

  if (status.ok()) {
    for (auto &x: crf_vocab) {

      // If the word frequency is greater than the threshold value and it not exists in 
      // the original vocabulary 
      if (x.second > thres_freq && original_vocab.find(x.first) == original_vocab.end()) {
        const char *y = classifier->Classify(ExtractNameFeature(x.first.c_str()));

        // And if it is not a name 
        if (strcmp(y, "F") == 0) {  
          candidates[x.first] = -log(static_cast<float>(x.second) / total_count);
        }
      }
    }
  }

  delete name_model;
  delete classifier;

  return candidates;
}