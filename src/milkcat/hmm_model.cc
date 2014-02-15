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
// hmm_model.h --- Created at 2013-12-05
//

#include "milkcat/hmm_model.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "utils/utils.h"
#include "utils/readable_file.h"

namespace milkcat {

#pragma pack(1)
struct HMMEmitRecord {
  int32_t term_id;
  int32_t tag_id;
  float cost;
};
#pragma pack(0)

HMMModel *HMMModel::New(const char *model_path, Status *status) {
  HMMModel *self = new HMMModel();
  ReadableFile *fd = ReadableFile::New(model_path, status);

  int32_t magic_number;
  if (status->ok()) fd->ReadValue<int32_t>(&magic_number, status);

  if (magic_number != 0x3322)
    *status = Status::Corruption(model_path);

  int32_t tag_num;
  if (status->ok()) fd->ReadValue<int32_t>(&tag_num, status);

  int32_t max_term_id;
  if (status->ok()) fd->ReadValue<int32_t>(&max_term_id, status);

  int32_t emit_num;
  if (status->ok()) fd->ReadValue<int32_t>(&emit_num, status);

  self->tag_str_ = reinterpret_cast<char (*)[16]>(new char[16 * tag_num]);
  for (int i = 0; i < tag_num && status->ok(); ++i) {
    fd->Read(self->tag_str_[i], 16, status);
  }

  self->transition_matrix_ = new double[tag_num * tag_num];
  float f_weight;
  for (int i = 0; i < tag_num * tag_num && status->ok(); ++i) {
    fd->ReadValue<float>(&f_weight, status);
    self->transition_matrix_[i] = f_weight;
  }

  self->emit_matrix_ = new EmitRow *[max_term_id + 1];
  memset(self->emit_matrix_, 0, sizeof(EmitRow *) * (max_term_id + 1));
  HMMEmitRecord emit_record;
  EmitRow *emit_node;
  for (int i = 0; i < emit_num && status->ok(); ++i) {
    fd->ReadValue<HMMEmitRecord>(&emit_record, status);
    emit_node = new EmitRow();
    emit_node->tag = emit_record.tag_id;
    emit_node->cost = emit_record.cost;
    emit_node->next = self->emit_matrix_[emit_record.term_id];
    self->emit_matrix_[emit_record.term_id] = emit_node;
  }

  if (status->ok() && fd->Tell() != fd->Size())
    *status = Status::Corruption(model_path);

  self->tag_num_ = tag_num;
  self->max_term_id_ = max_term_id;

  delete fd;
  if (!status->ok()) {
    delete self;
    return NULL;

  } else {
    return self;
  }
}


HMMModel::HMMModel(): emit_matrix_(NULL),
                      transition_matrix_(NULL),
                      tag_str_(NULL) {}

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

}  // namespace milkcat
