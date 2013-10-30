//
// crfpp_model.h --- Created at 2013-10-28
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
// Part of this code comes from CRF++
//
// CRF++ -- Yet Another CRF toolkit
// Copyright(C) 2005-2007 Taku Kudo <taku@chasen.org>
//

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "crfpp_model.h"
#include "utils.h"

static inline const char *read_pointer(const char **ptr, size_t size) {
  const char *r = *ptr;
  *ptr += size;
  return r;
}

template <class T> 
static inline void read_value(const char **ptr, T *value) {
  const char *r = read_pointer(ptr, sizeof(T));
  memcpy(value, r, sizeof(T));
}

CrfppModel::CrfppModel(): data_(NULL), cost_num_(0), cost_data_(NULL),
                          double_array_(NULL), cost_factor_(0.0) {
}

CrfppModel::~CrfppModel() {
  // cost_data_ and y_ are point to the area of data_
  // so it is unnecessary to delete them

  if (data_ != NULL) {
    delete data_;
    data_ = NULL;
  }

  if (double_array_ != NULL) {
    delete double_array_;
    double_array_ = NULL;
  }
}

CrfppModel *CrfppModel::Create(const char *model_path) {
  char error_message[1024];
  CrfppModel *self = new CrfppModel();

  FILE *fd = fopen(model_path, "rb");
  if (fd == NULL) {
    sprintf(error_message, "unable to open CRF++ model file %s", model_path);
    set_error_message(error_message);
    delete self;
    return NULL;
  }

  fseek(fd, 0L, SEEK_END);
  int file_size = ftell(fd);
  fseek(fd, 0L, SEEK_SET);
  self->data_ = new char[file_size];
  if (file_size != fread(self->data_, sizeof(char), file_size, fd)) {
    sprintf(error_message, "unable to read from CRF++ model file %s", model_path);
    set_error_message(error_message);
    delete self;
    return NULL;
  }
  fclose(fd);

  const char *ptr = self->data_,
             *end = ptr + file_size;

  int32_t version = 0;
  read_value<int32_t>(&ptr, &version);
  if (version != 100) {
    sprintf(error_message, "invalid model version %d", version);
    set_error_message(error_message);
    delete self;
    return NULL;    
  }

  int32_t type = 0;
  read_value<int32_t>(&ptr, &type);
  read_value<double>(&ptr, &(self->cost_factor_));
  read_value<int32_t>(&ptr, &(self->cost_num_));
  read_value<int32_t>(&ptr, &(self->xsize_));

  int32_t dsize = 0;
  read_value<int32_t>(&ptr, &dsize);

  int32_t y_str_size;
  read_value<int32_t>(&ptr, &y_str_size);
  const char *y_str = read_pointer(&ptr, y_str_size);
  int pos = 0;
  while (pos < y_str_size) {
    self->y_.push_back(y_str + pos);
    while (y_str[pos++] != '\0') {}
  }

  // ignore template data
  int32_t tmpl_str_size;
  read_value<int32_t>(&ptr, &tmpl_str_size);
  const char *tmpl_str = read_pointer(&ptr, tmpl_str_size);
  pos = 0;
  while (pos < tmpl_str_size) {
    const char *v = tmpl_str + pos;
    if (v[0] == '\0') {
      ++pos;
    } else if (v[0] == 'U') {
      self->unigram_templs_.push_back(v);
    } else if (v[0] == 'B') {
      self->bigram_templs_.push_back(v);
    } else {
      assert(false);
    }
    while (tmpl_str[pos++] != '\0') {}
  }
  
  self->double_array_ = new Darts::DoubleArray();
  self->double_array_->set_array(const_cast<char *>(ptr));
  ptr += dsize;

  self->cost_data_ = reinterpret_cast<const float *>(ptr);
  ptr += sizeof(float) * self->cost_num_;

  if (ptr != end) {
    sprintf(error_message, "CRF++ model file %s is broken", model_path);
    set_error_message(error_message);
    delete self;
    return NULL;    
  }

  return self;
}