/*
 * utils.h
 *
 * by ling0322 at 2013-08-10
 *
 */



#ifndef UTILS_H
#define UTILS_H 

#include <stdio.h>
#include "status.h"

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
            TypeName(const TypeName&); \
            void operator=(const TypeName&)

int token_type_to_word_type(int token_type);

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif // HAVE_STRLCPY

char *trim(char *str);

// open a random access file for read
class RandomAccessFile {
 public:
  static RandomAccessFile *New(const char *file_path, Status &status);

  ~RandomAccessFile();

  // Read n bytes from file
  bool Read(void *ptr, int size, Status &status);

  // Read an type T from file
  template<typename T>
  bool ReadValue(T &data, Status &status) {
    return Read(&data, sizeof(T), status);
  }

  // Read a line from file, if failed return false
  bool ReadLine(char *buf, int size, Status &status);

  bool Eof() { return ftell(fd_) == size_; }

  // Get current position in file
  int Tell();

  // Get the file size
  int Size() { return size_; }

 private:
  FILE *fd_;
  int size_;
  std::string file_path_;

  RandomAccessFile();
};


#endif // UTILS_H