/*
 * instance.h
 * instance_data.h
 *
 * by ling0322 at 2013-10-20
 *
 */

#ifndef INSTANCE_DATA_H
#define INSTANCE_DATA_H

#include <assert.h>
#include <string.h>
#include "milkcat_config.h"
#include "utils.h"


class InstanceData {
 public:
  InstanceData(int string_number, int integer_number, int capability);
  ~InstanceData();

  // Get the string of string_id at the position of this instance
  const char *string_at(int position, int string_id) const {
    assert(position < size_ && string_id < string_number_);
    return string_data_[string_id][position];
  }

  // Set the string of string_id at the position of this instance
  void set_string_at(int position, int string_id, const char *string_val) {
    assert(string_id < string_number_);
    strlcpy(string_data_[string_id][position], string_val, kFeatureLengthMax);
  }

  // Get the integer of integer_id at the position of this instance
  const int integer_at(int position, int integer_id) const {
    assert(position < size_ && integer_id < integer_number_);
    return integer_data_[integer_id][position];
  }

  // Set the integer of integer_id at the position of this instance
  void set_integer_at(int position, int integer_id, int integer_val) {
    assert(integer_id < integer_number_);
    integer_data_[integer_id][position] = integer_val;
  }

  // Get the size of this instance
  int size() const { return size_; }

  // Set the size of this instance
  void set_size(int size) { size_ = size; }

 private:
  char ***string_data_;
  int **integer_data_;
  int string_number_;
  int integer_number_;
  int size_;
  int capability_;

  DISALLOW_COPY_AND_ASSIGN(InstanceData);
};


#endif