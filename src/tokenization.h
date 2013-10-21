#ifndef TOKENIZATION_H
#define TOKENIZATION_H

#include "token_lex.h"

class TokenInstance;

class Tokenization {
 public:
  Tokenization(): buffer_alloced_(false) {}

  ~Tokenization();

  // Scan an string to get tokens
  void Scan(const char *buffer_string) {
    if (buffer_alloced_ == true) {
      yy_delete_buffer(yy_buffer_state_);
    }
    buffer_alloced_ = true;
    yy_buffer_state_ = yy_scan_string(buffer_string);
  }

  bool GetSentence(TokenInstance *token_instance);
  
private:
  YY_BUFFER_STATE yy_buffer_state_;
  bool buffer_alloced_;
};

#endif