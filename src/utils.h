/*
 * utils.h
 *
 * by ling0322 at 2013-08-10
 *
 */



#ifndef UTILS_H
#define UTILS_H 

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
            TypeName(const TypeName&); \
            void operator=(const TypeName&)

// Print an error message and exit when a fatal error occured
void set_error_message(const char *message);
const char *get_error_message();

int token_type_to_word_type(int token_type);

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif // HAVE_STRLCPY

char *trim(char *str);



#endif // UTILS_H