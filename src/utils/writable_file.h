//
// writable_file.h --- Created at 2014-02-03
//
// The MIT License (MIT)
//
// Copyright 2014 ling0322 <ling032x@gmail.com>
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

#ifndef WRITABLE_FILE_H
#define WRITABLE_FILE_H

#include <stdio.h>
#include <string>
#include "utils/status.h"

class WritableFile {
 public:
  // Open a file for write. On success, return an instance of WritableFile.
  // On failed, set status != Status::OK()
  static WritableFile *New(const char *path, Status &status);
  ~WritableFile();

  // Writes a line to file
  void WriteLine(const char *line, Status &status);

 private:
  FILE *fd_;
  std::string file_path_;

  WritableFile();
};

#endif