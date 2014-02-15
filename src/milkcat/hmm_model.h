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

#ifndef SRC_MILKCAT_HMM_MODEL_H_
#define SRC_MILKCAT_HMM_MODEL_H_

#include "utils/status.h"

namespace milkcat {

class HMMModel {
 public:
  struct EmitRow;

  static HMMModel *New(const char *model_path, Status *status);
  ~HMMModel();

  int tag_num() const { return tag_num_; }

  // Get Tag's string by its id
  const char *GetTagStr(int tag_id) const {
    return tag_str_[tag_id];
  }

  // Get the emit row (tag, cost) of a term, if no data return NULL
  EmitRow *GetEmitRow(int term_id) const {
    if (term_id > max_term_id_ || term_id < 0)
      return NULL;
    else
      return emit_matrix_[term_id];
  }

  // Get the transition cost from left_tag to right_tag
  double GetTransCost(int left_tag, int right_tag) const {
    return transition_matrix_[left_tag * tag_num_ + right_tag];
  }

 private:
  EmitRow **emit_matrix_;
  int max_term_id_;
  int tag_num_;
  char (* tag_str_)[16];
  double *transition_matrix_;

  HMMModel();
};

struct HMMModel::EmitRow {
  int tag;
  float cost;
  EmitRow *next;
};

}  // namespace milkcat

#endif  // SRC_MILKCAT_HMM_MODEL_H_
