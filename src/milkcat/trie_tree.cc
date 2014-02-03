//
// trie_tree.cc --- Created at 2013-12-05
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

#include <vector>
#include <algorithm>
#include "utils/utils.h"
#include "darts.h"
#include "trie_tree.h"
#include "milkcat_config.h"

DoubleArrayTrieTree *DoubleArrayTrieTree::New(const char *file_path, Status &status) {
  DoubleArrayTrieTree *self = new DoubleArrayTrieTree();

  if (-1 == self->double_array_.open(file_path)) {
    status = Status::IOError(file_path);
    delete self;
    return NULL;
  } else {
    return self;
  }
}

bool WordStrCmp(const char *w1, const char *w2) {
  return strcmp(w1, w2) < 0;
}

DoubleArrayTrieTree *DoubleArrayTrieTree::NewFromText(const char *file_path, Status &status) {
  DoubleArrayTrieTree *self = new DoubleArrayTrieTree();
  RandomAccessFile *fd = RandomAccessFile::New(file_path, status);
  std::vector<const char *> word_list;
  std::vector<int> user_id;
  char *p;

  while ((!fd->Eof()) && status.ok()) {
    p = new char[48];
    fd->ReadLine(p, 48, status);
    trim(p);
    // printf("%d\n", fd->Eof());

    if (status.ok()) {
      word_list.push_back(p);
      user_id.push_back(kUserTermId);     
    } else {
      delete p;
    }
  }

  // Sort the word_list before build double array
  if (status.ok()) std::sort(word_list.begin(), word_list.end(), WordStrCmp);

  // Not start to build the double array trietree, it should take some times if the 
  // file is big
  if (status.ok()) self->double_array_.build(word_list.size(), &word_list[0], NULL, &user_id[0]);

  for (std::vector<const char *>::iterator it = word_list.begin(); it != word_list.end(); ++it) {
    delete *it;
  }

  if (!status.ok()) {
    delete self;
    return NULL;
  } else {
    return self;
  }
}

int DoubleArrayTrieTree::Search(const char *text) const {
  return double_array_.exactMatchSearch<int>(text);
}

int DoubleArrayTrieTree::Traverse(const char *text, size_t &node) const {
  size_t key_pos = 0;
  return double_array_.traverse(text, node, key_pos);
}

