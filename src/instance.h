#ifndef INSTANCE_H
#define INSTANCE_H

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "milkcat_config.h"
#include "utils.h"


class Instance {
 public:
  Instance(int string_number, int integer_number, size_t capability): size_(0), feature_str_(NULL), feature_int_(NULL) {
    if (string_number != 0) {
      feature_str_ = new char **[string_number];
      for (int i = 0; i < string_number; ++i) {
        feature_str_[i] = new char *[capability];
        for (size_t j = 0; j < capability; ++j) {
          feature_str_[i][j] = new char[kFeatureLengthMax];
        }
      }
    }
    
    if (integer_number != 0) {
      feature_int_ = new int *[integer_number];
      for (int i = 0; i < integer_number; ++i) {
        feature_int_[i] = new int[capability];
      }
    }

    string_number_ = string_number;
    integer_number_ = integer_number;
    capability_ = capability;
  }

  virtual ~Instance() {
    if (feature_str_ != NULL) {
      for (int i = 0; i < string_number_; ++i) {
        for (size_t j = 0; j < capability_; ++j) {
          delete[] feature_str_[i][j];
        }
        delete[] feature_str_[i];
      } 
      delete[] feature_str_;
    }

    if (feature_int_ != NULL) {
      for (int i = 0; i < integer_number_; ++i) {
        delete[] feature_int_[i];
      }
      delete[] feature_int_;
    }
  }

  virtual const char *string_at(size_t position, int feature_id) const {
    // printf("pos:%d size:%d f:%d s:%d\n", position, size_, feature_id, string_number_);
    assert(position < size_ && feature_id < string_number_);
    return feature_str_[feature_id][position];
  }

  virtual void set_string_at(size_t position, int feature_id, const char *feature_str) {
    assert(feature_id < string_number_);
    strncpy(feature_str_[feature_id][position], feature_str, kFeatureLengthMax - 1);
  }

  virtual const int integer_at(size_t position, int feature_id) const {
    // printf("pos:%d size:%d f:%d s:%d\n", position, size_, feature_id, string_number_);
    assert(position < size_ && feature_id < integer_number_);
    return feature_int_[feature_id][position];
  }

  virtual void set_integer_at(size_t position, int feature_id, int feature_int) {
    assert(feature_id < integer_number_);
    feature_int_[feature_id][position] = feature_int;
  }

  virtual size_t size() const { return size_; }
  virtual void set_size(size_t size) { size_ = size; }
  virtual int string_number() const { return string_number_; }
  virtual int integer_number() const { return integer_number_; }

 private:
  char ***feature_str_;
  int **feature_int_;
  int string_number_;
  int integer_number_;
  size_t size_;
  size_t capability_;

  DISALLOW_COPY_AND_ASSIGN(Instance);
};


#endif