#include "token_instance.h"
#include "tokenization.h"
#include "token_lex.h"
#include "milkcat_config.h"

Tokenization::~Tokenization() {}

bool Tokenization::GetSentence(TokenInstance *token_instance) {
  int token_type;
  int token_count = 0;

  while (token_count < kTokenMax) {
    token_type = yylex();

    if (token_type == TokenInstance::kEnd) 
      break;
    
    token_instance->set_string_at(token_count, TokenInstance::kTokenUTF8S, yytext);
    token_instance->set_integer_at(token_count, TokenInstance::kTokenTypeI, token_type);
    
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
