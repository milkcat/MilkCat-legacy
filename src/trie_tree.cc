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

#include "darts.h"
#include "trie_tree.h"
#include "utils.h"

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

int DoubleArrayTrieTree::Search(const char *text) const {
  return double_array_.exactMatchSearch<int>(text);
}

int DoubleArrayTrieTree::Traverse(const char *text, size_t &node) const {
  size_t key_pos = 0;
  return double_array_.traverse(text, node, key_pos);
}
