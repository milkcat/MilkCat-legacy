//
// trie_tree.h --- Created at 2013-12-05
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

#ifndef TRIE_TREE_H
#define TRIE_TREE_H

#include "darts.h"
#include "utils.h"

class TrieTree {
 public:
  static const int kExist = -1;
  static const int kNone = -2;

  virtual ~TrieTree() = 0;

  // Search a trie tree if text exists return its value
  // else the return value is < 0
  virtual int Search(const char *text) const = 0;

  // Traverse a trie tree the node is the last state and will changed during traversing
  // for the root node the node = 0, return value > 0 if text exists 
  // returns kExist if something with text as its prefix exists buf text itself doesn't exist
  // return kNone it text doesn't exist.
  virtual int Traverse(const char *text, size_t &node) const = 0;
};

inline TrieTree::~TrieTree() {}

class DoubleArrayTrieTree: public TrieTree {
 public:
  static DoubleArrayTrieTree *Create(const char *file_path);
  DoubleArrayTrieTree() {}

  int Search(const char *text) const;
  int Traverse(const char *text, size_t &node) const;

 private:
  Darts::DoubleArray double_array_;

  DISALLOW_COPY_AND_ASSIGN(DoubleArrayTrieTree);
};

#endif