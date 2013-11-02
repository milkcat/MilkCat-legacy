/*
 * crf_pos_tagger.h
 *
 * by ling0322 at 2013-08-20
 *
 */


#ifndef CRF_POS_TAGGER_H
#define CRF_POS_TAGGER_H

#include "term_instance.h"
#include "part_of_speech_tag_instance.h"
#include "crf_tagger.h"
#include "milkcat_config.h" 

class FeatureExtractor;
class PartOfSpeechFeatureExtractor;

class CRFPOSTagger {
 public:
  static CRFPOSTagger *Create(const char *model_path);
  ~CRFPOSTagger();

  void Tag(PartOfSpeechTagInstance *part_of_speech_tag_instance, const TermInstance *term_instance);

 private:
  CRFTagger *crf_tagger_;
  CRFModel *crf_model_;

  CRFPOSTagger();
  PartOfSpeechFeatureExtractor *feature_extractor_;
  
  DISALLOW_COPY_AND_ASSIGN(CRFPOSTagger);
};



#endif