//
// utils.cc --- Created at 2013-08-31
//
// The MIT License (MIT)
//
// Copyright (c) 2013 ling0322 <ling032x@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "term_instance.h"
#include "token_instance.h"
#include "milkcat_config.h"
#include "utils.h"

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

char *trim(char *s) {
  char *p = s;
  int l = strlen(p);

  while(isspace(p[l - 1])) p[--l] = 0;
  while(* p && isspace(* p)) ++p, --l;

  memmove(s, p, l + 1);
  return s;
}


RandomAccessFile *RandomAccessFile::New(const char *file_path, Status &status) {
  RandomAccessFile *self = new RandomAccessFile();
  self->file_path_ = file_path;
  std::string msg;

  if ((self->fd_ = fopen(file_path, "rb")) != NULL) {
    fseek(self->fd_, 0, SEEK_END);
    self->size_ = ftell(self->fd_);
    fseek(self->fd_, 0, SEEK_SET);
    return self;

  } else {
    std::string msg("failed to open ");
    msg += file_path;
    status = Status::IOError(msg.c_str());
    return NULL;
  }
}

RandomAccessFile::RandomAccessFile(): fd_(NULL), size_(0) {}

bool RandomAccessFile::Read(void *ptr, int size, Status &status) {
  if (1 != fread(ptr, size, 1, fd_)) {
    std::string msg("failed to read from ");
    msg += file_path_;
    status = Status::IOError(msg.c_str());
    return false;    
  } else {
    return true;
  }
}

bool RandomAccessFile::ReadLine(char *ptr, int size, Status &status) {
  if (NULL == fgets(ptr, size, fd_)) {
    std::string msg("failed to read from ");
    msg += file_path_;
    status = Status::IOError(msg.c_str());
    return false;    
  } else {
    return true;
  }
}

RandomAccessFile::~RandomAccessFile() {
  if (fd_ != NULL) fclose(fd_);
}

int RandomAccessFile::Tell() { return ftell(fd_); }