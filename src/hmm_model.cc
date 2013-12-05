//
// hmm_model.h --- Created at 2013-12-05
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdexcept>
#include <fstream>
#include "utils.h"
#include "hmm_model.h"

#pragma pack(1)
struct HMMEmitRecord {
  int32_t term_id;
  int32_t tag_id;
  float cost;
};
#pragma pack(0)

HMMModel *HMMModel::Create(const char *model_path) {

  HMMModel *self = new HMMModel();

  try {
    std::ifstream ifs;
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
    ifs.open(model_path, std::ios::binary);
    
    // Get file size
    ifs.seekg(0, std::ios::end);
    int file_size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    int32_t magic_number;
    ifs.read(reinterpret_cast<char *>(&magic_number), sizeof(int32_t));

    if (magic_number != 0x3322)
      throw std::runtime_error(std::string("invalid HMM model file ") + model_path);

    int32_t tag_num;
    ifs.read(reinterpret_cast<char *>(&tag_num), sizeof(int32_t));

    int32_t max_term_id;
    ifs.read(reinterpret_cast<char *>(&max_term_id), sizeof(int32_t));

    int32_t emit_num;
    ifs.read(reinterpret_cast<char *>(&emit_num), sizeof(int32_t));

    self->tag_str_ = reinterpret_cast<char (*)[16]>(new char[16 * tag_num]);
    for (int i = 0; i < tag_num; ++i) {
      ifs.read(self->tag_str_[i], 16);
    }

    self->transition_matrix_ = new double[tag_num * tag_num];
    float f_weight;
    for (int i = 0; i < tag_num * tag_num; ++i) {
      ifs.read(reinterpret_cast<char *>(&f_weight), sizeof(float));
      self->transition_matrix_[i] = f_weight;
    }


    self->emit_matrix_ = new EmitRow *[max_term_id + 1];
    memset(self->emit_matrix_, 0, sizeof(EmitRow *) * (max_term_id + 1));
    HMMEmitRecord emit_record;
    EmitRow *emit_node;
    for (int i = 0; i < emit_num; ++i) {
      ifs.read(reinterpret_cast<char *>(&emit_record), sizeof(emit_record));
      emit_node = new EmitRow();
      emit_node->tag = emit_record.tag_id;
      emit_node->cost = emit_record.cost;
      emit_node->next = self->emit_matrix_[emit_record.term_id];
      self->emit_matrix_[emit_record.term_id] = emit_node;
    }

    if (ifs.tellg() != file_size)
      throw std::runtime_error(std::string("invalid HMM model file ") + model_path); 

    self->tag_num_ = tag_num;
    self->max_term_id_ = max_term_id;

    return self;

  } catch (std::ifstream::failure &ex) {
    std::string errmsg = std::string("failed to read from ") + model_path;
    set_error_message(errmsg.c_str());
    delete self;
    return NULL;

  } catch (std::exception &ex) {
    set_error_message(ex.what());
    delete self;
    return NULL;
  }
}


HMMModel::HMMModel(): emit_matrix_(NULL), transition_matrix_(NULL), tag_str_(NULL) {}

HMMModel::~HMMModel() {
  delete[] transition_matrix_;
  transition_matrix_ = NULL;

  delete[] tag_str_;
  tag_str_ = NULL;

  EmitRow *p, *q;
  if (emit_matrix_ != NULL) {
    for (int i = 0; i < max_term_id_ + 1; ++i) {
      p = emit_matrix_[i];
      while (p) {
        q = p->next;
        delete p;
        p = q;
      }
    }
  }
  delete[] emit_matrix_;
}
