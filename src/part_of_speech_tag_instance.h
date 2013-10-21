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
#include "instance_data.h"
#include "term_instance.h"
#include "tag_sequence.h"
#include "pos_tag_set.h"
#include "utils.h"

class PartOfSpeechTagInstance {
 public:
  PartOfSpeechTagInstance();
  ~PartOfSpeechTagInstance();

  static const int kPOSTagS = 0;

  const char *part_of_speech_tag_at(int position) const { 
    return instance_data_->string_at(position, kPOSTagS); 
  }

  // Set the size of this instance
  void set_size(int size) { instance_data_->set_size(size); }

  // Get the size of this instance
  int size() const { return instance_data_->size(); }

  // Set the value at position
  void set_value_at(int position, const char *tag) {
  	instance_data_->set_string_at(position, kPOSTagS, tag);
  }
 
 private:
  InstanceData *instance_data_;

  DISALLOW_COPY_AND_ASSIGN(PartOfSpeechTagInstance);
};


#endif