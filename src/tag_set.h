#ifndef TAG_TYPE_H
#define TAG_TYPE_H

class TagSet {
 public:
  typedef int TagId;

  virtual int size() = 0;
  virtual TagId TagStringToTagId(const char *) = 0;
  virtual const char *TagIdToTagString(TagId tag_id) = 0;
};



#endif