/*
  Darts -- Double-ARray Trie System

  mkdarts.cpp 1674 2008-03-22 11:21:34Z taku;
  Modified by ling0322 2013-06-04

  Copyright(C) 2001-2007 Taku Kudo <taku@chasen.org>
  All rights reserved.
*/

#include <algorithm>
#include <stdio.h>
#include <vector>
#include <string>
#include <darts.h>

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: mkdict [INPUT-FILE] [OUTPUT-FILE]\n");
    return 1;
  }

  const char *input_path = argv[argc - 2];
  const char *output_path = argv[argc - 1];

  Darts::DoubleArray double_array;
  
  FILE *fd = fopen(input_path, "r");
  if (fd == NULL) {
    fprintf(stderr, "error: unable to open input file %s\n", input_path);
    return 1;
  }

  char key_text[1024];
  int value;
  std::vector<std::pair<std::string, int> > key_value;
  while (fscanf(fd, "%s %d", key_text, &value) != EOF) {
    key_value.push_back(std::pair<std::string, int>(key_text, value));
  }
  sort(key_value.begin(), key_value.end());
  printf("read %ld words.\n", key_value.size());

  std::vector<const char *> keys;
  std::vector<Darts::DoubleArray::value_type> values;

  for (std::vector<std::pair<std::string, int> >::const_iterator it = key_value.begin(); it != key_value.end(); ++it) {
    keys.push_back(it->first.c_str());
    values.push_back(it->second);
  }

  if (double_array.build(keys.size(), &keys[0], 0, &values[0]) != 0) {
    fprintf(stderr, "error: unable to build double array from file %s\n", input_path);
    return 1;  
  }

  if (double_array.save(output_path) != 0) {
    fprintf(stderr, "error: unable to save double array to file %s\n", output_path);
    return 1; 
  };

  fclose(fd);
  return 0;
}
