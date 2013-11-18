#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "milkcat.h"

int print_usage() {
  fprintf(stderr, "Usage: milkcat [-m hmm_crf|crf|crf_seg] [-d model_directory] [-i] [-t] [file]\n");
  return 0;
}

const char *word_type_str(MC_WORD_TYPE word_type) {
  switch (word_type) {
   case MC_CHINESE_WORD:
    return "ZH";
   case MC_ENGLISH_WORD:
    return "EN";
   case MC_NUMBER:
    return "NUM";
   case MC_SYMBOL:
    return "SYM";
   case MC_PUNCTION:
    return "PU";
   case MC_OTHER:
    return "OTH";
  }

  return NULL;
}

int main(int argc, char **argv) {
  char model_path[1024] = "./";
  int method = NORMAL_PROCESSOR;
  char c;
  int errflag = 0;
  FILE *fp = NULL;
  int use_stdin_flag = 0;
  int display_tag = 1;
  int display_type = 0;

  while ((c = getopt (argc, argv, "itd:m:")) != -1) {
    switch (c) {
     case 'i':
      fp = stdin;
      use_stdin_flag = 1;
      break;

     case 'd':
      strcpy(model_path, optarg);
      if (model_path[strlen(model_path) - 1] != '/') 
        strcat(model_path, "/");
      break;

     case 'm':
      if (strcmp(optarg, "crf_seg") == 0) {
        method = CRF_SEGMENTER;
        display_tag = 0;
      } else if (strcmp(optarg, "crf") == 0) {
        method = CRF_PROCESSOR;
        display_tag = 1;
      } else if (strcmp(optarg, "hmm_crf") == 0) {
        method = NORMAL_PROCESSOR;
        display_tag = 1;
      } else if (strcmp(optarg, "hmm") == 0) {
        method = HMM_PROCESSOR;
        display_tag = 1;
      } else {
        errflag++;
      }
      break;

     case 't':
      display_type = 1;
      break;

     case ':':
      fprintf(stderr, "Option -%c requires an operand\n", optopt);
      errflag++;
      break;

     case '?':
      fprintf(stderr, "Unrecognized option: -%c\n", optopt);
      errflag++;
      break;
    }
  }

  if ((use_stdin_flag == 1 && argc - optind != 0) || (use_stdin_flag == 0 && argc - optind != 1)) {
    errflag++;
  }

  if (errflag) {
    print_usage();
    exit(2);
  }
  
  if (use_stdin_flag == 0) {
    fp = fopen(argv[optind], "r");
    if (fp == NULL) {
      fprintf(stderr, "Unable to open file: %s\n", argv[optind]);
      exit(1);
    }
  }

  char *input_buffer = (char *)malloc(1048576);
  milkcat_t *processor = milkcat_init(method, model_path);
  size_t sentence_length;
  int i;
  char ch;

  if (processor == NULL) {
    fputs(milkcat_get_error_message(), stderr);
    fputs("\n", stderr);
    return 1;
  }

  while (NULL != fgets(input_buffer, 1048576, fp)) {
    milkcat_process(processor, input_buffer);
    while (milkcat_next(processor) != 0) {
      // printf("22222222\n");
      switch (milkcat_get_word(processor)[0]) {
       case '\r':
       case '\n':
       case ' ':
        continue;
      }

      fputs(milkcat_get_word(processor), stdout);

      if (display_type == 1) {
        fputs("_", stdout);
        fputs(word_type_str(milkcat_get_word_type(processor)), stdout);
      }

      if (display_tag == 1) {
        fputs("/", stdout);
        fputs(milkcat_get_part_of_speech_tag(processor), stdout);
      }

      fputs("  ", stdout);
    }
    printf("\n");
  }

  milkcat_delete(processor);
  free(input_buffer);
  if (use_stdin_flag == 0)
    fclose(fp);
  return 0;
}