/*
  Darts -- Double-ARray Trie System

  mkdarts.cpp 1674 2008-03-22 11:21:34Z taku;
  Modified by ling0322 2012-06-04

  Copyright(C) 2001-2007 Taku Kudo <taku@chasen.org>
  All rights reserved.
*/

#include <cstdio>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <darts.h>

int progress_bar(size_t current, size_t total) {
  static char bar[] = "*******************************************";
  static int scale = sizeof(bar) - 1;
  static int prev = 0;

  int cur_percentage = static_cast<int>(100.0 * current / total);
  int bar_len = static_cast<int>(1.0 * current * scale / total);

  if (prev != cur_percentage) {
    printf("Making Double Array: %3d%% |%.*s%*s| ", cur_percentage, bar_len, bar, scale - bar_len, "");
    if (cur_percentage == 100) 
      printf("\n");
    else
      printf("\r");

    fflush(stdout);
  }

  prev = cur_percentage;
  return 1;
};



int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " FILE INDEX" << std::endl;
    return -1;
  }

  std::string file_path = argv[argc - 2];
  std::string index_path = argv[argc - 1];

  Darts::DoubleArray double_array;

  std::vector<const char *> key;
  td::vector<Darts::DoubleArray::value_type> term_id;
  std::vector<Darts::DoubleArray::value_type> term_id;
  std::ifstream input_stream(file_path);
  
  if (!input_stream) {
    std::cerr << "Cannot Open: " << file_path << std::endl;
    return -1;
  }

  std::string term;
  while (!input_stream.eof()) {
    input_stream >> term;
    char *tmp = new char[term.size() + 1];
    std::strcpy(tmp, term.c_str());
    term_id.push_back(key.size() + 1);
    key.push_back(tmp);
  }

  printf("Read %d terms.\n", key.size());

  if (double_array.build(key.size(), &key[0], 0, 0, &progress_bar) != 0 || double_array.save(index.c_str()) != 0) {
    std::cerr << "Error: cannot build double array " << file << std::endl;
    return -1;
  };

  for (unsigned int i = 0; i < key.size(); i++)
    delete [] key[i];

  std::cout << "Done!, Compression Ratio: " <<
    100.0 * da.nonzero_size() / da.size() << " %" << std::endl;

  return 0;
}
