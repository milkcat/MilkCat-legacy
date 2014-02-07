//
// bigram_anal.cc --- Created at 2014-01-30
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

#include <unordered_map>
#include <string>
#include <map>
#include <vector>
#include <utility>
#include <math.h>
#include <stdio.h>
#include "utils/readable_file.h"
#include "utils/status.h"
#include "milkcat/milkcat.h"

struct Adjacent {
  std::map<int, int> left;
  std::map<int, int> right;
};

constexpr int NOT_CJK = 1000000000;

int GetOrInsert(std::unordered_map<std::string, int> &dict, const std::string &word) {
  auto it = dict.find(word);
  if (it == dict.end()) {
    dict.insert(std::pair<std::string, int>(word, dict.size()));
    return dict.size();
  } else {
    return it->second;
  }
}

// Segment a string specified by text and return as a vector of int, which 
// is the id of word defined in dict. And update the vocabulary
std::vector<int> SegmentTextAndUpdateVocabulary(
      const char *text, 
      milkcat_t *analyzer,
      std::unordered_map<std::string, int> &dict,
      std::unordered_map<std::string, int> &vocab) {

  std::vector<int> line_words;

  milkcat_cursor_t cursor = milkcat_analyze(analyzer, text);
  milkcat_item_t item;

  while (milkcat_cursor_get_next(&cursor, &item) == MC_OK) {
    if (item.word_type == MC_CHINESE_WORD) {
      line_words.push_back(GetOrInsert(dict, item.word));
      vocab[item.word] += 1;
    } else {
      line_words.push_back(NOT_CJK);
      vocab["-NOT-CJK-"] += 1;
    }
  }

  return line_words;
} 

// Update the word_adjacent data from words specified by line_words
void UpdateAdjacent(std::vector<Adjacent> &word_adjacent, 
                    std::vector<int> &line_words,
                    int candidate_size) {
  int word_id;
  for (auto it = line_words.begin(); it != line_words.end(); ++it) {
    word_id = *it;
    if (word_id < candidate_size) {
      // OK, find a candidate word

      if (it != line_words.begin()) 
        word_adjacent[word_id].left[*(it - 1)] += 1;

      if (it != line_words.end() - 1) 
        word_adjacent[word_id].right[*(it + 1)] += 1;
    }
  }
}

// Calculates the adjacent entropy from word's adjacent word data
// specified by adjacent
double CalculateAdjacentEntropy(const std::map<int, int> &adjacent) {
  double entropy = 0, probability;
  int total_count = 0;
  for (auto &x: adjacent) {
    total_count += x.second;
  }

  for (auto &x: adjacent) {
    probability = static_cast<double>(x.second) / total_count;
    entropy += -probability * log(probability);
  }

  return entropy;
}

// Use bigram segmentation to analyze a corpus. Candidate to analyze is specified by 
// candidate, and the corpus is specified by corpus_path. It would use a temporary file
// called 'candidate_cost.txt' as user dictionary file for MilkCat.
// On success, stores the adjecent entropy in adjacent_entropy, and stores the vocabulary
// of segmentation's result in vocab. On failed set status != Status::OK()
void BigramAnalyze(const std::unordered_map<std::string, float> &candidate,
                   const char *corpus_path,
                   std::unordered_map<std::string, double> &adjacent_entropy,
                   std::unordered_map<std::string, int> &vocab,
                   Status &status) {

  std::unordered_map<std::string, int> dict;
  dict.reserve(500000);

  int candidate_size = candidate.size();
  std::vector<Adjacent> word_adjacent(candidate_size);

  // Put each candidate word into dictionary and assign its word-id
  for (auto &x: candidate) {
    dict.insert(std::pair<std::string, int>(x.first, dict.size()));
  }

  milkcat_model_t *model = milkcat_model_new(nullptr);
  milkcat_model_set_userdict(model, "candidate_cost.txt");
  milkcat_t *analyzer = milkcat_new(model, BIGRAM_SEGMENTER);
  if (analyzer == NULL) status = Status::Corruption(milkcat_last_error());

  ReadableFile *fd = nullptr;
  if (status.ok()) {
    fd = ReadableFile::New(corpus_path, status);
  }

  int buf_size = 1024 * 1024;
  char *buf = new char[buf_size];
  int word_id, left_word_id, right_word_id;
  std::vector<int> line_words;
  while (status.ok() && !fd->Eof()) {
    fd->ReadLine(buf, buf_size, status);

    if (status.ok()) {
      // Analyze the line from corpus, store it in line_words
      line_words = SegmentTextAndUpdateVocabulary(buf, analyzer, dict, vocab);

      // Scan the line_words, find the adjacent word-ids of candidate word
      UpdateAdjacent(word_adjacent, line_words, candidate_size);
    }
  }

  std::vector<std::string> id_to_str(dict.size());
  for (auto &x: dict) id_to_str[x.second] = x.first;

  // Calculate the candidates' adjacent entropy and store in adjacent_entropy
  for (auto it = word_adjacent.begin(); it != word_adjacent.end(); ++it) {
    double left_entropy = CalculateAdjacentEntropy(it->left);
    double right_entropy = CalculateAdjacentEntropy(it->right);
    adjacent_entropy[id_to_str[it - word_adjacent.begin()]] = std::min(left_entropy, right_entropy);
  }

  delete fd;
}