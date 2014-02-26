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

// Use the second order Hidden Markov Model (TnT Model) for tagging
// see also:
//     TnT -- A Statistical Part-of-Speech Tagger
//     http://aclweb.org/anthology//A/A00/A00-1031.pdf

#include "milkcat/hmm_model.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <unordered_map>
#include "milkcat/trie_tree.h"
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

  self->tag_str_ = reinterpret_cast<char (*)[kTagStrLenMax]>(
      new char[kTagStrLenMax * tag_num]);
  for (int i = 0; i < tag_num && status->ok(); ++i) {
    fd->Read(self->tag_str_[i], 16, status);
  }

  self->transition_matrix_ = new float[tag_num * tag_num];
  float f_weight;
  for (int i = 0; i < tag_num * tag_num && status->ok(); ++i) {
    fd->ReadValue<float>(&f_weight, status);
    self->transition_matrix_[i] = f_weight;
  }

  self->emits_ = new Emit *[max_term_id + 1];
  memset(self->emits_, 0, sizeof(Emit *) * (max_term_id + 1));
  HMMEmitRecord emit_record;
  Emit *emit_node;
  for (int i = 0; i < emit_num && status->ok(); ++i) {
    fd->ReadValue<HMMEmitRecord>(&emit_record, status);
    emit_node = new Emit(emit_record.tag_id, 
                         emit_record.cost, 
                         self->emits_[emit_record.term_id]);
    self->emits_[emit_record.term_id] = emit_node;
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

// Create the HMMModel instance from some text model file
HMMModel *HMMModel::NewFromText(const char *trans_model_path, 
                                const char *emit_model_path,
                                const char *yset_model_path,
                                const char *index_path,
                                Status *status) {
  char buff[1024];
  HMMModel *self = new HMMModel();


  // Get the y-tag set from file
  std::unordered_map<std::string, int> y_tag;
  ReadableFile *fd = ReadableFile::New(yset_model_path, status);
  while (status->ok() && !fd->Eof()) {
    fd->ReadLine(buff, sizeof(buff), status);
    if (status->ok()) {
      // Remove the LF character
      trim(buff);
      y_tag.emplace(buff, y_tag.size());
    }
  }
  delete fd;

  if (status->ok()) {
    self->tag_str_ = reinterpret_cast<char (*)[16]>(
        new char[kTagStrLenMax * y_tag.size()]);
    for (auto &x: y_tag) {
      strlcpy(self->tag_str_[x.second], x.first.c_str(), kTagStrLenMax);
    }
    self->tag_num_ = y_tag.size();

    int trans_matrix_size = self->tag_num_ * self->tag_num_ * self->tag_num_;
    self->transition_matrix_ = new float[trans_matrix_size];
  }

  // Get the transition data from trans_model file
  fd = nullptr;
  char leftleft_tagstr[1024] = "\0",
       left_tagstr[1024] = "\0",
       tagstr[1024] = "\0";
  double cost = 0;
  decltype(y_tag.begin()) it_leftleft, it_left, it_curr;
  if (status->ok()) fd = ReadableFile::New(trans_model_path, status);
  while (status->ok() && !fd->Eof()) {
    fd->ReadLine(buff, sizeof(buff), status);
    if (status->ok()) {
      sscanf(buff, 
             "%s %s %s %lf",
             leftleft_tagstr,
             left_tagstr,
             tagstr,
             &cost);
      it_leftleft = y_tag.find(leftleft_tagstr);
      it_left = y_tag.find(left_tagstr);
      it_curr = y_tag.find(tagstr);

      // If some tag is not in tag set
      if (it_curr == y_tag.end() || 
          it_left == y_tag.end() || 
          it_leftleft == y_tag.end()) {
        sprintf(buff, 
                "%s: Invalid tag trigram %s %s %s "
                "(one of them is not in tag set).", 
                trans_model_path,
                leftleft_tagstr,
                left_tagstr,
                tagstr);
        *status = Status::Corruption(buff);
      }
    }
    
    if (status->ok()) {
      int leftleft_tag = it_leftleft->second;
      int left_tag = it_left->second;
      int curr_tag = it_curr->second;
      self->transition_matrix_[leftleft_tag * self->tag_num_ * self->tag_num_ +
                               left_tag * self->tag_num_ +
                               curr_tag] = static_cast<float>(cost);
    }
  }
  delete fd;

  // Get emit data from file
  fd = nullptr;
  TrieTree *index = nullptr;
  std::unordered_map<int, Emit *> emit_map;
  char word[1024] = "\0";
  if (status->ok()) index = DoubleArrayTrieTree::New(index_path, status);
  if (status->ok()) fd = ReadableFile::New(emit_model_path, status);

  self->max_term_id_ = 0;
  int term_id;
  while (status->ok() && !fd->Eof()) {
    fd->ReadLine(buff, sizeof(buff), status);
    if (status->ok()) {
      sscanf(buff, 
             "%s %s %lf",
             tagstr,
             word,
             &cost);
      term_id = index->Search(word);
      if (term_id > self->max_term_id_) self->max_term_id_ = term_id;
      if (term_id < 0) continue;

      it_curr = y_tag.find(tagstr);
      if (it_curr == y_tag.end()) {
        sprintf(buff, 
                "%s: Invalid tag %s (not in tag set).", 
                emit_model_path,
                tagstr);
        *status = Status::Corruption(buff);
      }
    }

    if (status->ok()) {
      auto it_emit = emit_map.find(term_id);
      if (it_emit == emit_map.end()) {
        emit_map[term_id] = new Emit(it_curr->second, 
                                     static_cast<float>(cost),
                                     nullptr);
      } else {
        Emit *next = it_emit->second;
        emit_map[term_id] = new Emit(it_curr->second, 
                                     static_cast<float>(cost),
                                     next);
      }
    }
  }
  delete fd;

  if (status->ok()) {
    self->emits_ = new Emit *[self->max_term_id_ + 1];
    for (int i = 0; i <= self->max_term_id_; ++i) self->emits_[i] = nullptr;
    for (auto &x: emit_map) {
      self->emits_[x.first] = x.second;
    }
  }

  delete index;
  if (status->ok()) {
    return self;
  } else {
    delete self;
    return nullptr;
  }
}


HMMModel::HMMModel(): emits_(nullptr),
                      transition_matrix_(nullptr),
                      tag_str_(nullptr) {}

HMMModel::~HMMModel() {
  delete[] transition_matrix_;
  transition_matrix_ = nullptr;

  delete[] tag_str_;
  tag_str_ = nullptr;

  Emit *p, *q;
  if (emits_ != nullptr) {
    for (int i = 0; i < max_term_id_ + 1; ++i) {
      p = emits_[i];
      while (p) {
        q = p->next;
        delete p;
        p = q;
      }
    }
  }
  delete[] emits_;
}

}  // namespace milkcat
