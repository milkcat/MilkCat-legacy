#ifndef TERM_INSTANCE_H
#define TERM_INSTANCE_H

#include <stdio.h>
#include "instance_data.h"
#include "utils.h"

class TokenInstance;
class TagSequence;

class TermInstance {
 public:
  TermInstance();
  ~TermInstance();

  static const int kTermTextS = 0;
  static const int kTermTokenNumberI = 0;
  static const int kTermTypeI = 1;

  enum {
    kChineseWord = 0,
    kEnglishWord = 1,
    kNumber = 2,
    kSymbol = 3,
    kPunction = 4,
    kOther = 5
  };

  // Get the term's string value at position
  const char *term_text_at(int position) const { return instance_data_->string_at(position, kTermTextS); }

  // Get the term type at position
  int term_type_at(int position) const { return instance_data_->integer_at(position, kTermTypeI); }

  // Get the token number of this term at position
  int token_number_at(int position) const { return instance_data_->integer_at(position, kTermTokenNumberI); }

  // Set the size of this instance
  void set_size(int size) { instance_data_->set_size(size); }

  // Get the size of this instance
  int size() const { return instance_data_->size(); }

  // Set the value at position
  void set_value_at(int position, const char *term, int token_number, int term_type) {
    instance_data_->set_string_at(position, kTermTextS, term);
    instance_data_->set_integer_at(position, kTermTokenNumberI, token_number);
    instance_data_->set_integer_at(position, kTermTypeI, term_type);
  }

 private:
  InstanceData *instance_data_;

  DISALLOW_COPY_AND_ASSIGN(TermInstance);
};



#endif