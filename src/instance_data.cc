/*
 * instance_data.cc
 *
 * by ling0322 at 2013-10-20
 *
 */

#include "instance_data.h"

InstanceData::InstanceData(int string_number, int integer_number, int capability): 
    size_(0), 
    string_data_(NULL), 
    integer_data_(NULL) {

  if (string_number != 0) {
    string_data_ = new char **[string_number];
    for (int i = 0; i < string_number; ++i) {
      string_data_[i] = new char *[capability];
      for (size_t j = 0; j < capability; ++j) {
        string_data_[i][j] = new char[kFeatureLengthMax];
      }
    }
  }
  
  if (integer_number != 0) {
    integer_data_ = new int *[integer_number];
    for (int i = 0; i < integer_number; ++i) {
      integer_data_[i] = new int[capability];
    }
  }

  string_number_ = string_number;
  integer_number_ = integer_number;
  capability_ = capability;
}

InstanceData::~InstanceData() {
  if (string_data_ != NULL) {
    for (int i = 0; i < string_number_; ++i) {
      for (size_t j = 0; j < capability_; ++j) {
        delete[] string_data_[i][j];
      }
      delete[] string_data_[i];
    } 
    delete[] string_data_;
  }

  if (integer_data_ != NULL) {
    for (int i = 0; i < integer_number_; ++i) {
      delete[] integer_data_[i];
    }
    delete[] integer_data_;
  }
}