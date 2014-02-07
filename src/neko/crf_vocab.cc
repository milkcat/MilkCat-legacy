//
// crf_segmenet_vocabulary.cc --- Created at 2014-01-28
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

#include <unordered_map>
#include <string>
#include <vector>
#include <stdio.h>
#include "milkcat/milkcat.h"
#include "utils/readable_file.h"
#include "utils/status.h"

// Segment the corpus from path and return the vocabulary of chinese words.
// If any errors occured, status is not Status::OK()
std::unordered_map<std::string, int> GetCrfVocabulary(const char *path, int &total_count, Status &status) {
  std::unordered_map<std::string, int> vocab;
  milkcat_model_t *model = milkcat_model_new(NULL);
  milkcat_t *analyzer = milkcat_new(model, CRF_SEGMENTER);
  int buf_size = 1024 * 1024;
  char *buf = new char[buf_size];

  total_count = 0;

  if (analyzer == nullptr) status = Status::Corruption(milkcat_last_error());

  ReadableFile *fd;
  if (status.ok()) fd = ReadableFile::New(path, status);

  // Start to segment the file
  milkcat_item_t item;
  milkcat_cursor_t cursor;
  std::string word;
  while (status.ok() && fd->Eof() == false) {
    fd->ReadLine(buf, buf_size, status);
    cursor = milkcat_analyze(analyzer, buf);
    while (milkcat_cursor_get_next(&cursor, &item)) {
      if (item.word_type == MC_CHINESE_WORD)
        word = item.word;
      else
        word = "-NOT-CJK-";
      vocab[word] += 1;
      total_count++;
    }
  }

  delete fd;
  delete[] buf;
  milkcat_destroy(analyzer);
  milkcat_model_destroy(model);

  return vocab;
}
