#ifndef TOKEN_INSTANCE_H
#define TOKEN_INSTANCE_H

#include <assert.h>
#include "milkcat_config.h"
#include "instance.h"
#include "utils.h"


class TokenInstance: public Instance {
 public:
  TokenInstance(): Instance(1, 1, kTokenMax) {}
  ~TokenInstance() {}

  enum {
    kEnd = 0,
    kSpace = 1,
    kChineseChar = 2,
    kCrLf = 3,
    kPeriod = 4,
    kNumber = 5,
    kEnglishWord = 6,
    kPunctuation = 7,
    kSymbol = 8,
    kOther = 9
  };


  static const int kTokenTypeI = 0;
  static const int kTokenUTF8S = 0;
  static const int kTokenUTF8F = 0;

  DISALLOW_COPY_AND_ASSIGN(TokenInstance);
};


#endif