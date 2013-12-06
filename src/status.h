//
// status.h --- Created at 2013-12-06
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


#ifndef STATUS_H
#define STATUS_H

#include <string>

class Status {
 public:  

  enum {
    kIOError = 1,
    kCorruption = 2
  };

  Status(): code_(0), msg_("") {}
  Status(int code, const char *msg): code_(code), msg_(msg) {}
  Status(const Status &s): code_(s.code_), msg_(s.msg_) {}

  static Status OK() { return Status(); }

  static Status IOError(const char *msg) {
    return Status(kIOError, msg);
  }

  static Status Corruption(const char *msg) {
    return Status(kCorruption, msg);
  }

  bool ok() { return code_ == 0; }

  const char *what() { return msg_.c_str(); }

 private:
  int code_;
  std::string msg_;
};


#endif