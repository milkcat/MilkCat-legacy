/*
 * utils.cc
 *
 * by ling0322 at 2013-08-31
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "term_instance.h"
#include "token_instance.h"
#include "utils.h"

static char error_message_[2048] = "null";
void set_error_message(char *message) {
  strcpy(error_message_, message);
}

const char *get_error_message() {
  return error_message_;
}

int token_type_to_word_type(int token_type) {
  switch (token_type) {
   case TokenInstance::kChineseChar:
    return TermInstance::kChineseWord;
   case TokenInstance::kSpace:
   case TokenInstance::kCrLf:
    return TermInstance::kOther;
   case TokenInstance::kPeriod:
   case TokenInstance::kPunctuation:
   case TokenInstance::kOther:
    return TermInstance::kPunction;
   case TokenInstance::kEnglishWord:
    return TermInstance::kEnglishWord;
   case TokenInstance::kSymbol:
    return TermInstance::kSymbol;
   case TokenInstance::kNumber:
    return TermInstance::kNumber;
   default:
    return TermInstance::kOther;
  }
}