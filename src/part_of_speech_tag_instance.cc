/*
 * part_of_speech_tag_instance.cc
 *
 * by ling0322 at 2013-10-20
 *
 */

#include "part_of_speech_tag_instance.h"
#include "milkcat_config.h"

PartOfSpeechTagInstance::PartOfSpeechTagInstance() {
  instance_data_ = new InstanceData(1, 1, kTokenMax);
}
PartOfSpeechTagInstance::~PartOfSpeechTagInstance() {
  delete instance_data_;
}