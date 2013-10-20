#ifndef SEGMENT_TAG_SET_H
#define SEGMENT_TAG_SET_H

#include <string.h>
#include <assert.h>
#include "tag_set.h"

class SegmentTagSet: public TagSet {
 public:
  typedef int TagId;

  static const TagId B = 0;
  static const TagId B1 = 1;
  static const TagId B2 = 2;
  static const TagId M = 3;
  static const TagId E = 4;
  static const TagId S = 5;

  int size() { return 6; };

  TagId TagStringToTagId(const char *tag_string) {
    if (strcmp(tag_string, "B") == 0)
      return B;

    if (strcmp(tag_string, "B1") == 0)
      return B1;

    if (strcmp(tag_string, "B2") == 0)
      return B2;

    if (strcmp(tag_string, "M") == 0)
      return M;

    if (strcmp(tag_string, "E") == 0)
      return E;

    if (strcmp(tag_string, "S") == 0)
      return S;

    assert(false);
    return -1;
  }

  const char *TagIdToTagString(TagId tag_id) {
    if (tag_id == B)
      return "B";

    if (tag_id == B1)
      return "B1";

    if (tag_id == B2)
      return "B2";

    if (tag_id == M)
      return "M";

    if (tag_id == E)
      return "E";

    if (tag_id == S)
      return "S";

    assert(false);
    return "";
  }
};



#endif