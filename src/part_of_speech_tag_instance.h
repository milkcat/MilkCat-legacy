/*
 * part_of_speech_tag_instance.h
 * term_pos_tag_instance.h
 *
 * by ling0322 at 2013-10-10
 *
 */

#ifndef PART_OF_SPEECH_TAG_INSTANCE_H
#define PART_OF_SPEECH_TAG_INSTANCE_H

#include <assert.h>
#include "instance.h"
#include "term_instance.h"
#include "tag_sequence.h"
#include "pos_tag_set.h"
#include "utils.h"

class PartOfSpeechTagInstance: public Instance {
 public:
  PartOfSpeechTagInstance(): Instance(1, 0, kTokenMax) {}

  static const int kPOSTagS = 0;

  void set_value_at(int position, const char *tag) {
  	set_string_at(position, kPOSTagS, tag);
  }
 
 private:
  DISALLOW_COPY_AND_ASSIGN(PartOfSpeechTagInstance);
};


#endif