//
// static_array.h --- Created at 2013-12-06
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

#ifndef STATIC_ARRAY_H
#define STATIC_ARRAY_H

#include <fstream>
#include <stdexcept>
#include <assert.h>
#include "utils.h"

template<class T>
class StaticArray {
 public:
  static StaticArray *Load(const char *file_path) {
    int type_size = sizeof(T);
    StaticArray *self = new StaticArray();
    try {
      std::ifstream ifs;
      ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
      ifs.open(file_path, std::ios::binary);
      
      // Get file size
      ifs.seekg(0, std::ios::end);
      int file_size = ifs.tellg();
      ifs.seekg(0, std::ios::beg);

      if (file_size % type_size != 0) 
        throw std::runtime_error(std::string("invalid static-array file ") + file_path);

      self->data_ = new T[file_size / type_size];
      self->size_ = file_size / type_size;

      ifs.read(reinterpret_cast<char *>(self->data_), file_size);

    } catch (std::ifstream::failure &ex) {
      std::string errmsg = std::string("failed to read from ") + file_path;
      set_error_message(errmsg.c_str());
      delete self;
      return NULL;

    } catch (std::exception &ex) {
      set_error_message(ex.what());
      delete self;
      return NULL;
    }

    return self;
  }

  ~StaticArray() {
    delete data_;
    data_ = NULL;
  }

  T get(int position) {
    assert(position < size_);
    return data_[position];
  }

  int size() { return size_; }

 private:
  T *data_;
  int size_;
};

#endif