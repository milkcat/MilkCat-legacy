//
// mkgram.cc --- Created at 2013-10-21
// (with) mkdict.cc --- Created at 2013-06-08
// mk_model.cc -- Created at 2013-11-08
//
// The MIT License (MIT)
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
#include <algorithm>
#include <set>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <assert.h>
#include <darts.h>
#include "utils.h"
#include "static_hashtable.h"

#pragma pack(1)
struct BigramRecord {
  int32_t word_left;
  int32_t word_right;
  float weight;
};

struct HMMEmitRecord {
  int32_t term_id;
  int32_t tag_id;
  float weight;
};
#pragma pack(0)

#define UNIGRAM_INDEX_FILE "unigram.idx"
#define UNIGRAM_DATA_FILE "unigram.bin"
#define BIGRAM_FILE "bigram.bin"
#define HMM_MODEL_FILE "hmm_model.bin"

// Load unigram data from unigram_file, returns the number of entries successfully loaded
// if an error occured return -1
int LoadUnigramFile(const char *unigram_file, std::map<std::string, double> &unigram_data) {
  FILE *fp = fopen(unigram_file, "r");
  char word[100];
  double count,
         sum = 0;

  if (fp == NULL) {
    fprintf(stderr, "\nerror: unable to open unigram file %s\n", unigram_file);
    fflush(stderr);
    return -1;
  }

  while (EOF != fscanf(fp, "%s %lf", word, &count)) {
    unigram_data[std::string(word)] += count;
    sum += count;
  }

  // Calculate the weight = -log(freq / total)
  for (std::map<std::string, double>::iterator it = unigram_data.begin(); it != unigram_data.end(); ++it) {
    it->second = -log(it->second / sum);
  }

  fclose(fp);
  return unigram_data.size();
}

// Load bigram data from unigram_file, returns the number of entries successfully loaded
// if an error occured return -1
int LoadBigramFile(const char *bigram_file, 
                   std::map<std::pair<std::string, std::string>, int> &bigram_data,
                   int &total_count) {
  FILE *fp = fopen(bigram_file, "r");
  char left[100], right[100];
  int count; 

  total_count = 0;

  if (fp == NULL) {
    fprintf(stderr, "\nerror: unable to open bigram file %s\n", bigram_file);
    fflush(stderr);
    return -1;
  }
  
  while (EOF != fscanf(fp, "%s %s %d", left, right, &count)) {
    bigram_data[std::pair<std::string, std::string>(left, right)] += count;
    total_count += count;
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

// Save unigram data into binary file UNIGRAM_FILE. On success, return the number of 
// bigram word pairs successfully writed. On failed, return -1
int SaveBigramBinFile(const std::map<std::pair<std::string, std::string>, int> &bigram_data, 
                      int total_count,
                      const Darts::DoubleArray &double_array) {
  FILE *fp = fopen(BIGRAM_FILE, "wb");
  if (fp == NULL) {
    fprintf(stderr, "\nerror: unable to open %s for write.\n", BIGRAM_FILE);
    fflush(stderr);
    return -1;
  }

  BigramRecord bigram_record;
  const char *left_word, *right_word;
  int32_t left_id, right_id;
  int count;
  std::vector<int64_t> keys;
  std::vector<float> values;

  int write_num = 0;
  for (std::map<std::pair<std::string, std::string>, int>::const_iterator it = bigram_data.begin(); 
       it != bigram_data.end();  
       ++it) {
    left_word = it->first.first.c_str();
    right_word = it->first.second.c_str();
    count = it->second;
    left_id = double_array.exactMatchSearch<Darts::DoubleArray::value_type>(left_word);
    right_id = double_array.exactMatchSearch<Darts::DoubleArray::value_type>(right_word);
    if (left_id > 0 && right_id > 0) {
      keys.push_back((static_cast<int64_t>(left_id) << 32) + right_id);
      values.push_back(static_cast<float>(-log(static_cast<double>(count) / total_count)));
    }
  }

  const StaticHashTable<int64_t, float> *hashtable = StaticHashTable<int64_t, float>::Build(&keys[0], &values[0], keys.size());
  if (hashtable == 0) {
    fprintf(stderr, "\nerror: unable to build hash table.\n");
    fflush(stderr);    
  }
  if (hashtable->Save(BIGRAM_FILE) == false) {
    fprintf(stderr, "\nerror: unable to write to file %s.\n", BIGRAM_FILE);
    fflush(stderr);      
  }

  delete hashtable;
  fclose(fp);
  return keys.size();
}

int MakeGramModel(int argc, char **argv) {
  Darts::DoubleArray double_array;
  std::map<std::string, double> unigram_data;
  std::map<std::pair<std::string, std::string>, int> bigram_data;
  
  if (argc != 4) {
    fprintf(stderr, "Usage: mc_model gram [UNIGRAM FILE] [BIGRAM FILE]\n");
    fflush(stderr);
    return 1;
  }

  const char *unigram_file = argv[argc - 2];
  const char *bigram_file = argv[argc - 1];

  printf("Loading unigram data ...");
  fflush(stdout);
  int count = LoadUnigramFile(unigram_file, unigram_data);
  if (count == -1) {
    return 2;
  }
  printf(" OK, %d entries loaded.\n", count);

  printf("Loading bigram data ...");
  fflush(stdout);
  int total_count;
  count = LoadBigramFile(bigram_file, bigram_data, total_count);
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
  count = SaveBigramBinFile(bigram_data, total_count, double_array);
  if (count < 0) {
    return 7;
  }
  printf(" OK, %d entries saved.\n", count);
  printf("Success!");

  return 0;
}

int MakeIndexFile(int argc, char **argv) {
  if (argc != 4) {
    fprintf(stderr, "Usage: mc_model dict [INPUT-FILE] [OUTPUT-FILE]\n");
    return 1;
  }

  const char *input_path = argv[argc - 2];
  const char *output_path = argv[argc - 1];

  Darts::DoubleArray double_array;
  
  FILE *fd = fopen(input_path, "r");
  if (fd == NULL) {
    fprintf(stderr, "error: unable to open input file %s\n", input_path);
    return 1;
  }

  char key_text[1024];
  int value;
  std::vector<std::pair<std::string, int> > key_value;
  while (fscanf(fd, "%s %d", key_text, &value) != EOF) {
    key_value.push_back(std::pair<std::string, int>(key_text, value));
  }
  sort(key_value.begin(), key_value.end());
  printf("read %ld words.\n", key_value.size());

  std::vector<const char *> keys;
  std::vector<Darts::DoubleArray::value_type> values;

  for (std::vector<std::pair<std::string, int> >::const_iterator it = key_value.begin(); it != key_value.end(); ++it) {
    keys.push_back(it->first.c_str());
    values.push_back(it->second);
  }

  if (double_array.build(keys.size(), &keys[0], 0, &values[0]) != 0) {
    fprintf(stderr, "error: unable to build double array from file %s\n", input_path);
    return 1;  
  }

  if (double_array.save(output_path) != 0) {
    fprintf(stderr, "error: unable to save double array to file %s\n", output_path);
    return 1; 
  }

  fclose(fd);
  return 0;
}

//
// Build the HMM Tagger Model
//
// Model format is
//
// int32_t: magic_number = 0x3322
// int32_t: tag_num
// int32_t: max_term_id
// int32_t: emit_num
// char[16] * tag_num: tag_text
// float * tag_num * tag_num: transition matrix
// n * HMMEmitRecord: emit matrix
//
int MakeHMMTaggerModel(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "Usage: mc_model hmm_tag [TAGSET-FILE] [EMIT-FILE] [TRANS-FILE] [INDEX-FILE]\n");
    return 1;
  }

  const char *tagset_path = argv[argc - 4];
  const char *emit_path = argv[argc - 3];
  const char *trans_path = argv[argc - 2];
  const char *index_path = argv[argc - 1];

  FILE *fd_output = fopen(HMM_MODEL_FILE, "w");
  if (fd_output == NULL) {
    fprintf(stderr, "error: unable to open %s for write.\n", HMM_MODEL_FILE);
    return 1; 
  }

  int32_t magic_number = 0x3322;
  if (1 != fwrite(&magic_number, sizeof(int32_t), 1, fd_output)) {
    fprintf(stderr, "error: unable to write to file %s.\n", HMM_MODEL_FILE);
    return 1;     
  }

  // Get tagset from tagset_file
  char line_buf[1024];
  FILE *fd_tagset = fopen(tagset_path, "r");
  int count = 0;
  std::map<std::string, int> tag_id_map;
  std::vector<std::string> tag_str;
  if (NULL == fd_tagset) {
    fprintf(stderr, "error: unable to open %s for read.\n", tagset_path);
    return 1;         
  }
  while (NULL != fgets(line_buf, 1024, fd_tagset)) {
    trim(line_buf);
    if (strlen(line_buf) > 15) {
      fprintf(stderr, "error: invalid tag length (>15) %s.\n", line_buf);
      return 1;   
    }

    tag_id_map[line_buf] = count++;
    tag_str.push_back(line_buf);
  }
  fclose(fd_tagset);
  printf("get %d tags from tagset file.\n", count);
  fflush(stdout);

  // Put tagset data into model file
  int32_t tag_number = static_cast<int32_t>(count);
  if (1 != fwrite(&tag_number, sizeof(int32_t), 1, fd_output)) {
    fprintf(stderr, "error: unable to write to file %s.\n", HMM_MODEL_FILE);
    return 1;     
  }

  // Write max_term_id and emit_num later, temporarily 0
  int32_t max_term_id = 0;
  if (1 != fwrite(&max_term_id, sizeof(int32_t), 1, fd_output)) {
    fprintf(stderr, "error: unable to write to file %s.\n", HMM_MODEL_FILE);
    return 1;     
  }
  int32_t emit_num = 0;
  if (1 != fwrite(&emit_num, sizeof(int32_t), 1, fd_output)) {
    fprintf(stderr, "error: unable to write to file %s.\n", HMM_MODEL_FILE);
    return 1;     
  }
  for (std::vector<std::string>::iterator it = tag_str.begin(); it != tag_str.end(); ++it) {
    memset(line_buf, 0, 16);
    strlcpy(line_buf, it->c_str(), 16);
    if (16 != fwrite(&line_buf, sizeof(char), 16, fd_output)) {
      fprintf(stderr, "error: unable to write to file %s.\n", HMM_MODEL_FILE);
      return 1;     
    }
  }

  // Read transition matrix file data
  printf("build transition matrix ...");
  fflush(stdout);
  FILE *fd_trans = fopen(trans_path, "r");
  if (fd_trans == NULL) {
    fprintf(stderr, "error: unable to open %s for read.\n", trans_path);
    return 1;
  }

  char tag_left[128],
       tag_right[128];
  double log_prob;
  float *trans_matrix_data = new float[tag_number * tag_number];
  for (int i = 0; i < tag_number * tag_number; ++i) {
    trans_matrix_data[i] = 1e37;
  }
  while (EOF != fscanf(fd_trans, "%s %s %lf", tag_left, tag_right, &log_prob)) {
    if (tag_id_map.find(tag_left) == tag_id_map.end() || tag_id_map.find(tag_right) == tag_id_map.end()) {
      fprintf(stderr, "error: invalid tag pair (not in tagset) %s %s.\n", tag_left, tag_right);
      return 1;
    }
    trans_matrix_data[tag_id_map[tag_left] * tag_number + tag_id_map[tag_right]] = static_cast<float>(log_prob);
  }
  if (tag_number * tag_number != fwrite(trans_matrix_data, sizeof(float), tag_number * tag_number, fd_output)) {
    fprintf(stderr, "error: unable to write to file %s.\n", HMM_MODEL_FILE);
    return 1;    
  }
  delete[] trans_matrix_data;
  fclose(fd_trans);
  printf("OK\n");
  fflush(stdout);


  // Read transition matrix file data
  printf("build emit matrix ...");
  fflush(stdout);
  Darts::DoubleArray double_array;
  if (-1 == double_array.open(index_path)) {
    fprintf(stderr, "error: unable to open index file %s.\n", index_path);
    return 1;  
  }
  FILE *fd_emit = fopen(emit_path, "r");
  count = 0;
  int ignore_count = 0;
  HMMEmitRecord record;
  while (EOF != fscanf(fd_emit, "%s %s %lf", line_buf, tag_left, &log_prob)) {
    int32_t term_id = double_array.exactMatchSearch<Darts::DoubleArray::value_type>(line_buf);
    if (term_id < 0) {
      ignore_count++;
      continue;
    }
    if (term_id > max_term_id) {
      max_term_id = term_id;
    }
    record.term_id = term_id;
    count++;
    if (tag_id_map.find(tag_left) == tag_id_map.end()) {
      fprintf(stderr, "error: invalid tag (not in tagset) %s.\n", tag_left);
      return 1;
    }
    record.tag_id = tag_id_map[tag_left];
    record.weight = static_cast<float>(log_prob);
    if (1 != fwrite(&record, sizeof(record), 1, fd_output)) {
      fprintf(stderr, "error: unable to write to file %s.\n", HMM_MODEL_FILE);
      return 1;  
    }
    emit_num++;
  }
  printf("OK write %d records, ignore %d records\n", count, ignore_count);
  fflush(stdout);

  // Come back and write max_term_id and emit_size
  if (0 != fseek(fd_output, 8, SEEK_SET)) {
    fprintf(stderr, "error: unable to write to file %s.\n", HMM_MODEL_FILE);
    return 1;   
  }
  if (1 != fwrite(&max_term_id, sizeof(int32_t), 1, fd_output)) {
    fprintf(stderr, "error: unable to write to file %s.\n", HMM_MODEL_FILE);
    return 1;      
  }
  if (1 != fwrite(&emit_num, sizeof(int32_t), 1, fd_output)) {
    fprintf(stderr, "error: unable to write to file %s.\n", HMM_MODEL_FILE);
    return 1;      
  }

  printf("Success!\n");
  fflush(stdout);

  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: mc_model [dict|gram|hmm]\n");
    return 1;
  }

  char *tool = argv[1];

  if (strcmp(tool, "dict") == 0) {
    return MakeIndexFile(argc, argv);
  } else if (strcmp(tool, "gram") == 0) {
    return MakeGramModel(argc, argv);
  } else if (strcmp(tool, "hmm") == 0) {
    return MakeHMMTaggerModel(argc, argv);
  } else {
    fprintf(stderr, "Usage: mc_model [dict|gram|hmm]\n");
    return 1;    
  }

  return 0;
}