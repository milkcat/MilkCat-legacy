#ifndef TERM_INSTANCE_H
#define TERM_INSTANCE_H

#include <stdio.h>
#include "instance.h"
#include "utils.h"

class TokenInstance;
class TagSequence;

class TermInstance: public Instance {
 public:
  TermInstance(): Instance(3, 2, kTokenMax) {}
  ~TermInstance() {}

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

  void set_value_at(int position, const char *term, int token_number, int term_type) {
    set_string_at(position, kTermTextS, term);
    set_integer_at(position, kTermTokenNumberI, token_number);
    set_integer_at(position, kTermTypeI, term_type);
  }

 private:
  char buffer_[1000];

  DISALLOW_COPY_AND_ASSIGN(TermInstance);
};



#endif