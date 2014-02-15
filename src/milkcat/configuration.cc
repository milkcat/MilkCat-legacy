//
// The MIT License (MIT)
//
// Copyright 2013-2014 The MilkCat Project Developers
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
// configuration.cc --- Created at 2013-11-08
//

#include "milkcat/configuration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "utils/utils.h"

namespace milkcat {

Configuration::Configuration() {}

Configuration *Configuration::New(const char *path, Status *status) {
  char line[2048],
       key[2048],
       value[2048],
       *p,
       error_messge[1024];
  int line_number = 0;
  FILE *fp = fopen(path, "r");
  Configuration *self = new Configuration();

  if (fp == NULL) {
    *status = Status::IOError(path);
  }

  while (status->ok() && NULL != fgets(line, 2048, fp)) {
    line_number++;
    trim(line);
    if (line[0] == '#') continue;

    p = line;
    while (*p != '=' && *p != '\0') p++;
    if (*p == '\0') {
      snprintf(error_messge,
               sizeof(error_messge),
               "missing '=' in %s line %d.",
               path,
               line_number);
      *status = Status::Corruption(error_messge);
    }

    if (status->ok()) {
      strlcpy(key, line, p - line + 1);
      strlcpy(value, p + 1, 1024);
      trim(key);
      trim(value);

      self->data_[key] = value;
    }
  }

  if (fp != NULL) fclose(fp);

  if (status->ok()) {
    return self;
  } else {
    delete self;
    return NULL;
  }
}

bool Configuration::SaveToPath(const char *path) {
  FILE *fp = fopen(path, "w");
  if (fp == NULL) return false;

  for (auto &x : data_) {
    fprintf(fp, "%s = %s\n", x.first.c_str(), x.second.c_str());
  }

  fclose(fp);
  return true;
}

int Configuration::GetInteger(const char *key) const {
  return atoi(GetString(key));
}

bool Configuration::HasKey(const char *key) const {
  return data_.find(key) != data_.end();
}

const char *Configuration::GetString(const char *key) const {
  auto it = data_.find(std::string(key));
  if (it != data_.end()) {
    return it->second.c_str();
  } else {
    return "";
  }
}

void Configuration::SetInteger(const char *key, int value) {
  char buf[1024];
  snprintf(buf, sizeof(buf), "%d", value);
  data_[key] = buf;
}

void Configuration::SetString(const char *key, const char * value) {
  data_[key] = value;
}

}  // namespace milkcat
