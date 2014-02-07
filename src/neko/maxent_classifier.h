//
// maxent_classifier.h --- Created at 2014-01-31
//
// The MIT License (MIT)
//
// Copyright (c) 2014 ling0322 <ling032x@gmail.com>
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

#ifndef MAXENT_CLASSIFIER_H
#define MAXENT_CLASSIFIER_H

#include <unordered_map>
#include <string>
#include <vector>
#include "utils/status.h"

class MaxentModel {
 public:
  // Load and create the maximum entropy model data from the model file specified 
  // by model_path. On success, return the instance and status = Status::OK(). On 
  // failed, return nullptr and status != Status::OK()
  static MaxentModel *New(const char *model_path, Status &status);

  // Get the cost of a feature with its y-tag in maximum entropy model
  float cost(int yid, int feature_id) {
    return model_[yid][feature_id];
  }

  // If the feature_str not exists in feature set, use this value instead
  static constexpr int kFeatureIdNone = -1;

  // Get the number of y-tags in the model
  int ysize() const { return yname_.size(); }

  // Get the name string of a y-tag's id
  const char *yname(int yid) const { return yname_[yid].c_str(); }

  // Return the id of feature_str, if the feature_str not in feature set of model
  // return kFeatureIdNone
  int feature_id(const char *feature_str) const {
    auto it = feature_ids_.find(std::string(feature_str));
    if (it == feature_ids_.end())
      return kFeatureIdNone;
    else
      return it->second;
  }

 private:
  std::unordered_map<std::string, int> feature_ids_;
  std::unordered_map<std::string, int> yid_;
  std::vector<std::string> yname_;
  std::unordered_map<int, std::unordered_map<int, float>> model_;
};

class MaxentClassifier {
 public:
  MaxentClassifier(MaxentModel *model);
  ~MaxentClassifier();

  // Classify an instance of features specified by feature_list and return the y-tag
  // for the instance
  const char *Classify(const std::vector<std::string> &feature_list) const;

 private:
  MaxentModel *model_;
  double *y_cost_;
  int y_size_;
};

#endif