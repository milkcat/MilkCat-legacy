#ifndef TOKEN_INSTANCE_H
#define TOKEN_INSTANCE_H

#include <assert.h>
#include "milkcat_config.h"
#include "instance_data.h"
#include "utils.h"

class TokenInstance {
 public:
  TokenInstance();
  ~TokenInstance();

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

  // Get the token's string value at position
  virtual const char *token_text_at(int position) const { return instance_data_->string_at(position, kTokenUTF8S); }

  // Get the token type at position
  virtual int token_type_at(int position) const { return instance_data_->integer_at(position, kTokenTypeI); }

  // Set the size of this instance
  void set_size(int size) { instance_data_->set_size(size); }

  // Get the size of this instance
  virtual int size() const { return instance_data_->size(); }

  // Set the value at position
  void set_value_at(int position, const char *token_text, int token_type) { 
    instance_data_->set_string_at(position, kTokenUTF8S, token_text);
    instance_data_->set_integer_at(position, kTokenTypeI, token_type);
  }

 private:
  InstanceData *instance_data_;
  DISALLOW_COPY_AND_ASSIGN(TokenInstance);
};


#endif