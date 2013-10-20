#ifndef TAG_SEQUENCE_H
#define TAG_SEQUENCE_H

#include <assert.h>
#include <stdio.h>
#include "tag_set.h"

class TagSequence {
 public:
  TagSequence(size_t capability): capability_(capability), tags_(new TagSet::TagId[capability]) {}

  ~TagSequence() {
    delete[] tags_;
    tags_ = NULL;
  }

  TagSet::TagId GetTagAt(size_t tag_pos) const {
    // printf("%d %d\n", tag_pos, length_);
    assert(tag_pos < length_);
    return tags_[tag_pos];
  }

  void SetTagAt(size_t tag_pos, TagSet::TagId tag) {
    assert(tag_pos < length_);
    // printf("%d %s\n", tag_pos, tag);
    tags_[tag_pos] = tag;
  }

  size_t length() const { return length_; }
  void set_length(size_t length) {
    assert(length <= capability_);
    length_ = length;
  }

 private:
  TagSet::TagId *tags_;
  size_t length_;
  size_t capability_;
};

#endif