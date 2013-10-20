#ifndef POS_TAG_SET_H
#define POS_TAG_SET_H

#include "assert.h"
#include "tag_set.h"
#include "string.h"

class POSTagSet: public TagSet {
 public:
  int size() { return 35; }

  TagId TagStringToTagId(const char *tag_name) {
    for (TagId tag_id = 0; tag_id < size(); ++tag_id) {
      if (strcmp(pos_tag_name_[tag_id], tag_name) == 0)
        return tag_id;
    }

    assert(false);
    return 0;
  }

  const char *TagIdToTagString(TagId tag_id) {
    assert(tag_id < size());
    return pos_tag_name_[tag_id];
  }
 
 private:
  static const char pos_tag_name_[35][10];
};


#endif