
#include <stdio.h>
#include "token_instance.h"
#include "tokenizer.h"
#include "token_lex.h"
#include "milkcat_config.h"

Tokenization::~Tokenization() {
  if (buffer_alloced_ == true) {
    yy_delete_buffer(yy_buffer_state_);
  }
  yylex_destroy();
}

bool Tokenization::GetSentence(TokenInstance *token_instance) {
  int token_type;
  int token_count = 0;

  while (token_count < kTokenMax - 1) {
    token_type = yylex();

    if (token_type == TokenInstance::kEnd) 
      break;
    
    token_instance->set_value_at(token_count, yytext, token_type);
    token_count++;
    
    if (token_type == TokenInstance::kPeriod || token_type == TokenInstance::kCrLf) 
      break;
  }

  token_instance->set_size(token_count);
  return token_count != 0;
}

int yywrap() {
  return 1;
}
