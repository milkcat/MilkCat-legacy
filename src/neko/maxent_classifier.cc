//
// maxent_classifier.cc --- Created at 2014-02-01
//
// The MIT License (MIT)
//
// Copyright 2014 ling0322 <ling032x@gmail.com>
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
#include <stdlib.h>
#include <algorithm>
#include "utils/readable_file.h"
#include "utils/status.h"
#include "maxent_classifier.h"

MaxentModel *MaxentModel::New(const char *model_path, Status &status) {
  ReadableFile *fd = ReadableFile::New(model_path, status);
  MaxentModel *self = new MaxentModel();
  char line[1024];

  char y[1024], x[1024];
  int xid, yid;
  float cost;
  while (status.ok() && !fd->Eof()) {
    fd->ReadLine(line, 1024, status);
    if (status.ok()) {
      sscanf(line, "%s %s %f", y, x, &cost);
      if (self->yid_.find(y) == self->yid_.end()) {
        self->yid_.insert(std::pair<std::string, int>(y, self->yid_.size()));
        self->yname_.push_back(y);
      }
      if (self->feature_ids_.find(x) == self->feature_ids_.end())
        self->feature_ids_[x] = self->feature_ids_.size();

      xid = self->feature_ids_[x];
      yid = self->yid_[y];

      self->model_[yid][xid] = cost;
    }
  }

  delete fd;
  if (status.ok()) {
    return self;
  } else {
    delete self;
    return nullptr;
  }
}

MaxentClassifier::MaxentClassifier(MaxentModel *model): 
    model_(model),
    y_cost_(new double[model->ysize()]),
    y_size_(model->ysize()) {
}

MaxentClassifier::~MaxentClassifier() {
  delete y_cost_;
  y_cost_ = nullptr;
}

const char *MaxentClassifier::Classify(const std::vector<std::string> &feature_list) const {
  // Clear the y_cost_ array
  for (int i = 0; i < y_size_; ++i) y_cost_[i] = 0.0;

  for (auto &feature_str: feature_list) {
    int x = model_->feature_id(feature_str.c_str());
    if (x != MaxentModel::kFeatureIdNone) {
      // If this feature exists
      for (int y = 0; y < y_size_; ++y) {
        // printf("%d %d\n", y, x);
        y_cost_[y] += model_->cost(y, x);
      }
    }
  }

  // To find the maximum cost of y
  double *maximum_y = std::max_element(y_cost_, y_cost_ + y_size_);
  int maximum_yid = maximum_y - y_cost_;
  return model_->yname(maximum_yid);
}