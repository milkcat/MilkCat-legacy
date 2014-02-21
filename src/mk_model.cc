//
// The MIT License (MIT)
//
// Copyright 2013-2014 The MilkCat Project Developers
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
// mkgram.cc --- Created at 2013-10-21
// (with) mkdict.cc --- Created at 2013-06-08
// mk_model.cc -- Created at 2013-11-08
// mctools.cc -- Created at 2014-02-21
//

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <assert.h>
#include <unistd.h>
#include <map>
#include <string>
#include <algorithm>
#include <thread>
#include <set>
#include "common/get_vocabulary.h"
#include "utils/utils.h"
#include "utils/readable_file.h"
#include "utils/writable_file.h"
#include "milkcat/milkcat.h"
#include "milkcat/static_hashtable.h"
#include "milkcat/darts.h"
#include "neko/maxent_classifier.h"

namespace milkcat {

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

// Load unigram data from unigram_file, if an error occured set status !=
// Status::OK()
std::map<std::string, double>
LoadUnigramFile(const char *unigram_file, Status *status) {
  ReadableFile *fd = ReadableFile::New(unigram_file, status);
  std::map<std::string, double> unigram_data;
  char word[1024], line[1024];
  double count, sum = 0;

  while (status->ok() && !fd->Eof()) {
    fd->ReadLine(line, sizeof(line), status);
    if (status->ok()) {
      sscanf(line, "%s %lf", word, &count);
      unigram_data[std::string(word)] += count;
      sum += count;
    }
  }

  // Calculate the weight = -log(freq / total)
  if (status->ok()) {
    for (auto it = unigram_data.begin(); it != unigram_data.end(); ++it) {
      it->second = -log(it->second / sum);
    }
  }

  delete fd;
  return unigram_data;
}

// Load bigram data from bigram_file, if an error occured set status !=
// Status::OK()
std::map<std::pair<std::string, std::string>, int>
LoadBigramFile(const char *bigram_file, int *total_count, Status *status) {
  std::map<std::pair<std::string, std::string>, int> bigram_data;
  char left[100], right[100], line[1024];
  int count;

  ReadableFile *fd = ReadableFile::New(bigram_file, status);
  *total_count = 0;

  while (status->ok() && !fd->Eof()) {
    fd->ReadLine(line, sizeof(line), status);
    if (status->ok()) {
      sscanf(line, "%s %s %d", left, right, &count);
      bigram_data[std::pair<std::string, std::string>(left, right)] += count;
      *total_count += count;
    }
  }

  delete fd;
  return bigram_data;
}

// Build Double-Array TrieTree index from unigram, and save the index and the
// unigram data file
void BuildAndSaveUnigramData(const std::map<std::string, double> &unigram_data,
                             Darts::DoubleArray *double_array,
                             Status *status) {
  std::vector<const char *> key;
  std::vector<Darts::DoubleArray::value_type> term_id;
  std::vector<float> weight;

  // term_id = 0 is reserved for out-of-vocabulary word
  int i = 1;
  weight.push_back(0.0);

  for (auto &x : unigram_data) {
    key.push_back(x.first.c_str());
    term_id.push_back(i++);
    weight.push_back(x.second);
  }

  int result = double_array->build(key.size(), &key[0], 0, &term_id[0]);
  if (result != 0) *status = Status::RuntimeError("unable to build trie-tree");

  WritableFile *fd = nullptr;
  if (status->ok()) fd = WritableFile::New(UNIGRAM_DATA_FILE, status);
  if (status->ok())
    fd->Write(weight.data(), sizeof(float) * weight.size(), status);


  if (status->ok()) {
    if (0 != double_array->save(UNIGRAM_INDEX_FILE)) {
      std::string message = "unable to save index file ";
      message += UNIGRAM_INDEX_FILE;
      *status = Status::RuntimeError(message.c_str());
    }
  }

  delete fd;
}

// Save unigram data into binary file UNIGRAM_FILE. On success, return the
// number of bigram word pairs successfully writed. On failed, set status !=
// Status::OK()
int SaveBigramBinFile(
    const std::map<std::pair<std::string, std::string>, int> &bigram_data,
    int total_count,
    const Darts::DoubleArray &double_array,
    Status *status) {

  BigramRecord bigram_record;
  const char *left_word, *right_word;
  int32_t left_id, right_id;
  int count;
  std::vector<int64_t> keys;
  std::vector<float> values;

  int write_num = 0;
  for (auto &x : bigram_data) {
    left_word = x.first.first.c_str();
    right_word = x.first.second.c_str();
    count = x.second;
    left_id = double_array.exactMatchSearch<int>(left_word);
    right_id = double_array.exactMatchSearch<int>(right_word);
    if (left_id > 0 && right_id > 0) {
      keys.push_back((static_cast<int64_t>(left_id) << 32) + right_id);
      values.push_back(-log(static_cast<double>(count) / total_count));
    }
  }

  auto hashtable = StaticHashTable<int64_t, float>::Build(
      keys.data(),
      values.data(),
      keys.size());
  hashtable->Save(BIGRAM_FILE, status);

  delete hashtable;
  return keys.size();
}

int MakeGramModel(int argc, char **argv) {
  Darts::DoubleArray double_array;
  std::map<std::string, double> unigram_data;
  std::map<std::pair<std::string, std::string>, int> bigram_data;
  Status status;

  if (argc != 4)
    status = Status::Info("Usage: mc_model gram [UNIGRAM FILE] [BIGRAM FILE]");

  const char *unigram_file = argv[argc - 2];
  const char *bigram_file = argv[argc - 1];

  if (status.ok()) {
    printf("Loading unigram data ...");
    fflush(stdout);
    unigram_data = LoadUnigramFile(unigram_file, &status);
  }

  int total_count = 0;
  if (status.ok()) {
    printf(" OK, %d entries loaded.\n", static_cast<int>(unigram_data.size()));
    printf("Loading bigram data ...");
    fflush(stdout);
    bigram_data = LoadBigramFile(bigram_file, &total_count, &status);
  }

  if (status.ok()) {
    printf(" OK, %d entries loaded.\n", static_cast<int>(bigram_data.size()));
    printf("Saveing unigram index and data file ...");
    fflush(stdout);
    BuildAndSaveUnigramData(unigram_data, &double_array, &status);
  }

  int count = 0;
  if (status.ok()) {
    printf(" OK\n");
    printf("Saving Bigram Binary File ...");
    count = SaveBigramBinFile(bigram_data, total_count, double_array, &status);
  }

  if (status.ok()) {
    printf(" OK, %d entries saved.\n", count);
    printf("Success!");
    return 0;
  } else {
    puts(status.what());
    return -1;
  }
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

  for (auto &x : key_value) {
    keys.push_back(x.first.c_str());
    values.push_back(x.second);
  }

  if (double_array.build(keys.size(), &keys[0], 0, &values[0]) != 0) {
    fprintf(stderr,
            "error: unable to build double array from file %s\n",
            input_path);
    return 1;
  }

  if (double_array.save(output_path) != 0) {
    fprintf(stderr,
            "error: unable to save double array to file %s\n",
            output_path);
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
    fprintf(stderr, "Usage: mc_model hmm_tag [TAGSET-FILE]"
                    " [EMIT-FILE] [TRANS-FILE] [INDEX-FILE]\n");
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
  for (auto &x : tag_str) {
    memset(line_buf, 0, 16);
    strlcpy(line_buf, x.c_str(), 16);
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
    if (tag_id_map.find(tag_left) == tag_id_map.end() ||
        tag_id_map.find(tag_right) == tag_id_map.end()) {
      fprintf(stderr,
              "error: invalid tag pair (not in tagset) %s %s.\n",
              tag_left,
              tag_right);
      return 1;
    }
    int id = tag_id_map[tag_left] * tag_number + tag_id_map[tag_right];
    trans_matrix_data[id] = static_cast<float>(log_prob);
  }
  int result = fwrite(trans_matrix_data,
                      sizeof(float),
                      tag_number * tag_number,
                      fd_output);
  if (tag_number * tag_number != result) {
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
    int32_t term_id = double_array.exactMatchSearch<int>(line_buf);
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

int MakeMaxentFile(int argc, char **argv) {
  Status status;

  if (argc != 4)
    status = Status::Info("Usage: mc_model maxent "
                          "text-model-file binary-model-file");

  printf("Load text formatted model: %s \n", argv[argc - 2]);
  MaxentModel *maxent = MaxentModel::NewFromText(argv[argc - 2], &status);

  if (status.ok()) {
    printf("Save binary formatted model: %s \n", argv[argc - 1]);
    maxent->Save(argv[argc - 1], &status);
  }

  delete maxent;
  if (status.ok()) {
    return 0;
  } else {
    puts(status.what());
    return -1;
  }
}

void DisplayProgress(int64_t bytes_processed,
                     int64_t file_size,
                     int64_t bytes_per_second) {
  fprintf(stderr,
          "\rprogress %dMB/%dMB -- %2.1f%% %.3fMB/s",
          static_cast<int>(bytes_processed / (1024 * 1024)),
          static_cast<int>(file_size / (1024 * 1024)),
          100.0 * bytes_processed / file_size,
          bytes_per_second / static_cast<double>(1024 * 1024));
}

int CorpusVocabulary(int argc, char **argv) {
  Status status;
  char message[1024], 
       output_file[1024] = "",
       model_path[1024] = "",
       user_dict[1024] = "";
  int c = '\0';
  int analyzer_type = BIGRAM_SEGMENTER;

  while ((c = getopt(argc, argv, "u:d:m:o:")) != -1 && status.ok()) {
    switch (c) {
      case 'd':
        strcpy(model_path, optarg);
        if (model_path[strlen(model_path) - 1] != '/') 
          strcat(model_path, "/");
        break;

      case 'u':
        strcpy(user_dict, optarg);
        break;

      case 'o':
        strcpy(output_file, optarg);
        break;

      case 'm':
        if (strcmp(optarg, "crf_seg") == 0) {
          analyzer_type = CRF_SEGMENTER;
        } else if (strcmp(optarg, "bigram_seg") == 0) {
          analyzer_type = BIGRAM_SEGMENTER;
        } else {
          status = Status::Info("Option -m: invalid method");
        }
        break;

      case ':':
        sprintf(message, "Option -%c: requires an operand\n", optopt);
        status = Status::Info(message);
        break;

      case '?':
        sprintf(message, "Unrecognized option: -%c\n", optopt);
        status = Status::Info(message);
        break;
    }
  }

  if (status.ok() && argc - optind != 1) {
    status = Status::Info("");
  }

  if (!status.ok()) {
    if (*status.what()) puts(status.what());
    puts("Usage: mctools vocab [-m crf_seg|bigram_seg] [-d model_dir] [-u userdict] -o output_file corpus_file");
  }

  milkcat_model_t *model = nullptr;
  std::unordered_map<std::string, int> vocab;
  if (status.ok()) {
    const char *corpus_path = argv[optind];
    int total_count = 0;
    model = milkcat_model_new(*model_path == '\0'? NULL: model_path);
    if (*user_dict) milkcat_model_set_userdict(model, user_dict);
    int n_threads = std::thread::hardware_concurrency();
    vocab = GetVocabularyFromFile(corpus_path,
                                  model,
                                  analyzer_type,
                                  n_threads,
                                  &total_count,
                                  DisplayProgress,
                                  &status);
  }

  WritableFile *fd = nullptr;
  if (status.ok()) {
    fd = WritableFile::New(output_file, &status);
  }

  if (status.ok()) {
    for (auto &x: vocab) {
      sprintf(message, "%s %d", x.first.c_str(), x.second);
      fd->WriteLine(message, &status);
      if (!status.ok()) break;
    }
  }

  delete fd;
  milkcat_model_destroy(model);

  if (status.ok()) {
    puts("Success!");
    return 0;
  } else {
    if (*status.what()) puts(status.what());
    return -1;
  }
}

}  // namespace milkcat

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: mc_model [dict|gram|hmm|maxent]\n");
    return 1;
  }

  char *tool = argv[1];

  if (strcmp(tool, "dict") == 0) {
    return milkcat::MakeIndexFile(argc, argv);
  } else if (strcmp(tool, "gram") == 0) {
    return milkcat::MakeGramModel(argc, argv);
  } else if (strcmp(tool, "hmm") == 0) {
    return milkcat::MakeHMMTaggerModel(argc, argv);
  } else if (strcmp(tool, "maxent") == 0) {
    return milkcat::MakeMaxentFile(argc, argv);
  } else if (strcmp(tool, "vocab") == 0) {
    return milkcat::CorpusVocabulary(argc - 1, argv + 1);
  } else {
    fprintf(stderr, "Usage: mc_model [dict|gram|hmm|maxent|vocab]\n");
    return 1;
  }

  return 0;
}

