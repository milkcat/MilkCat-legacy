//
// make_bigram_model.cc --- Created at 2013-10-21
//
// Copyright (c) 2013 ling0322 <ling032x@gmail.com>
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

#include <map>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <darts.h>

#pragma pack(1)
struct BigramRecord {
  int32_t word_left;
  int32_t word_right;
  float weight;
};
#pragma pack(0)

#define UNIGRAM_INDEX_FILE "unigram.idx"
#define UNIGRAM_DATA_FILE "unigram.bin"
#define BIGRAM_FILE "bigram.bin"

// Load unigram data from unigram_file, returns the number of entries successfully loaded
// if an error occured return -1
int LoadUnigramFile(const char *unigram_file, std::map<std::string, double> &unigram_data) {
  FILE *fp = fopen(unigram_file, "r");
  char word[100];
  double weight;

  if (fp == NULL) {
    fprintf(stderr, "\nerror: unable to open unigram file %s\n", unigram_file);
    fflush(stderr);
    return -1;
  }

  while (EOF != fscanf(fp, "%s %lf", word, &weight)) {
    unigram_data.insert(std::pair<std::string, double>(std::string(word), weight));
  }

  fclose(fp);
  return unigram_data.size();
}

// Load bigram data from unigram_file, returns the number of entries successfully loaded
// if an error occured return -1
int LoadBigramFile(const char *bigram_file, 
                   std::map<std::pair<std::string, std::string>, double> &bigram_data) {
  FILE *fp = fopen(bigram_file, "r");
  char left[100], right[100];
  double weight;

  if (fp == NULL) {
    fprintf(stderr, "\nerror: unable to open bigram file %s\n", bigram_file);
    fflush(stderr);
    return -1;
  }
  
  while (EOF != fscanf(fp, "%s %s %lf", left, right, &weight)) {
    bigram_data.insert(std::pair<std::pair<std::string, std::string>, double>(
        std::pair<std::string, std::string>(std::string(left), std::string(right)), weight));
  }

  fclose(fp);  
  return bigram_data.size();
}

// Build Double-Array TrieTree index from unigram, and save the index and the unigram data file
int BuildAndSaveUnigramData(const std::map<std::string, double> &unigram_data, 
                            Darts::DoubleArray &double_array) {
  std::vector<const char *> key;
  std::vector<Darts::DoubleArray::value_type> term_id;
  std::vector<float> weight;

  // term_id = 0 is reserved for out-of-vocabulary word
  int i = 1;
  weight.push_back(0.0);

  for (std::map<std::string, double>::const_iterator it = unigram_data.begin(); it != unigram_data.end(); ++it) {
    key.push_back(it->first.c_str());
    term_id.push_back(i++);
    weight.push_back(it->second);
  }


  int result = double_array.build(key.size(), &key[0], 0, &term_id[0]);
  if (result != 0) {
    fprintf(stderr, "\nerror: unable to build trie-tree\n");
    fflush(stderr);
  }

  FILE *fp = fopen(UNIGRAM_DATA_FILE, "wb");
  if (fp == NULL) {
    fprintf(stderr, "\nerror: unable to open %s for write.\n", UNIGRAM_DATA_FILE);
    fflush(stderr);
    return -1;
  }

  if (weight.size() != fwrite(&weight[0], sizeof(float), weight.size(), fp)) {
    fprintf(stderr, "\nerror: unable to write to file %s.\n", UNIGRAM_DATA_FILE);
    fflush(stderr);
    return -1;     
  }
  fclose(fp);
  
 
  if (0 != double_array.save(UNIGRAM_INDEX_FILE)) {
    fprintf(stderr, "\nerror: unable to save double array file %s\n", UNIGRAM_INDEX_FILE);
    fflush(stderr);
    return 5;
  }

  return result;
}

// Save unigram data into binary file UNIGRAM_FILE, returns 0 if successfully saved
int SaveBigramBinFile(const std::map<std::pair<std::string, std::string>, double> &bigram_data, 
                      const Darts::DoubleArray &double_array) {
  FILE *fp = fopen(BIGRAM_FILE, "wb");
  if (fp == NULL) {
    fprintf(stderr, "\nerror: unable to open %s for write.\n", BIGRAM_FILE);
    fflush(stderr);
    return -1;
  }

  BigramRecord bigram_record;
  const char *left_word, *right_word;
  int left_id, right_id;
  double weight;
  for (std::map<std::pair<std::string, std::string>, double>::const_iterator it = bigram_data.begin(); 
       it != bigram_data.end();  
       ++it) {
    left_word = it->first.first.c_str();
    right_word = it->first.second.c_str();
    weight = it->second;
    left_id = double_array.exactMatchSearch<Darts::DoubleArray::value_type>(left_word);
    right_id = double_array.exactMatchSearch<Darts::DoubleArray::value_type>(right_word);
    if (left_id < 0 || right_id < 0) {
      fprintf(stderr, "\nerror: unable to get word_id of %s and %s.\n", left_word, right_word);
      fflush(stderr);
      return -1;
    }

    bigram_record.word_left = static_cast<int32_t>(left_id);
    bigram_record.word_right = static_cast<int32_t>(right_id);
    bigram_record.weight = static_cast<float>(weight);

    if (1 != fwrite(&bigram_record, sizeof(BigramRecord), 1, fp)) {
      fprintf(stderr, "\nerror: unable to write to file %s.\n", BIGRAM_FILE);
      fflush(stderr);
      return -1;     
    }
  }

  fclose(fp);
  return 0;
}

int main(int argc, char **argv) {
  Darts::DoubleArray double_array;
  std::map<std::string, double> unigram_data;
  std::map<std::pair<std::string, std::string>, double> bigram_data;
  
  if (argc != 3) {
    fprintf(stderr, "Usage: mkbigram [UNIGRAM FILE] [BIGRAM FILE]\n");
    fflush(stderr);
    return 1;
  }

  const char *unigram_file = argv[1];
  const char *bigram_file = argv[2];

  printf("Loading unigram data ...");
  fflush(stdout);
  int count = LoadUnigramFile(unigram_file, unigram_data);
  if (count == -1) {
    return 2;
  }
  printf(" OK, %d entries loaded.\n", count);

  printf("Loading bigram data ...");
  fflush(stdout);
  count = LoadBigramFile(bigram_file, bigram_data);
  if (count == -1) {
    return 3;
  }
  printf(" OK, %ld entries loaded.\n", bigram_data.size());

  printf("Saveing unigram index and data file ...");
  fflush(stdout);
  int result = BuildAndSaveUnigramData(unigram_data, double_array);
  if (result != 0) {
    return 4;
  }
  printf(" OK\n");

  printf("Saving Bigram Binary File ...");
  if (0 != SaveBigramBinFile(bigram_data, double_array)) {
    return 7;
  }
  printf(" OK\nSuccess!");

  return 0;
}