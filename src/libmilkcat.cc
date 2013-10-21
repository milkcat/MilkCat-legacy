//
// Yaca: Yet Another Chinese Segmenter, Part-Of-Speech Tagger and Dependency Parser
//
// libyaca.cc 
// Created at 2013-09-03
//


#include <string>
#include <string.h>
#include <stdio.h>
#include "milkcat.h"
#include "crf_tagger.h"
#include "tokenization.h"
#include "utils.h"
#include "tag_set.h"
#include "segment_tag_set.h"
#include "term_instance.h"
#include "token_instance.h"
#include "tag_sequence.h"
#include "pos_tag_set.h"
#include "part_of_speech_tag_instance.h"
#include "hmm_segment_and_pos_tagger.h"
#include "out_of_vocabulary_word_recognitioin.h"



class MilkCatProcessor {
 public:
  MilkCatProcessor(): tokenization_(NULL),
                      token_instance_(NULL) {
  }

  virtual ~MilkCatProcessor() {
    if (tokenization_ != NULL) {
      delete tokenization_;
      tokenization_ = NULL;
    }

    if (token_instance_ != NULL) {
      delete token_instance_;
      token_instance_ = NULL;
    }
  }

  virtual bool Initialize() {
    tokenization_ = new Tokenization();
    token_instance_ = new TokenInstance();

    return true;
  }

  virtual int NextSentence() {
    if (tokenization_->GetSentence(token_instance_) == false) 
      return 0;
    else
      return 1;
  }

  virtual void Process(const char *text) { tokenization_->Scan(text); }
  virtual size_t SentenceLength() { return 0; }
  virtual const char *GetTerm(int position) { return NULL; }
  virtual const char *GetPartOfSpeechTag(int position) { return NULL; }
  virtual int GetWordType(int position) { return -1; }
  virtual const char *GetRepr(int position) { return NULL; }

 protected:
  Tokenization *tokenization_;
  TokenInstance *token_instance_;
};

class MilkCatHMMSegPOSTaggerProcessor: public MilkCatProcessor {
 public:
  static MilkCatHMMSegPOSTaggerProcessor *Create(const char *model_dir_path) {
    MilkCatHMMSegPOSTaggerProcessor *self = new MilkCatHMMSegPOSTaggerProcessor();
    if (self->Initialize(model_dir_path) == false) {
      delete self;
      return NULL;
    } else {
      return self;
    }
  }

  bool Initialize(const char *model_dir_path) {
    if (MilkCatProcessor::Initialize() == false)
      return false;

    term_instance_ = new TermInstance();
    part_of_speech_tag_instance_ = new PartOfSpeechTagInstance();

    hmm_segment_and_pos_tagger_ = HMMSegmentAndPOSTagger::Create(
        (std::string(model_dir_path) + "term.darts").c_str(), 
        (std::string(model_dir_path) + "pos_term_emit.model").c_str(), 
        (std::string(model_dir_path) + "pos_tag_trans.model").c_str());

    if (hmm_segment_and_pos_tagger_ == NULL) 
      return false;

    return true;
  } 

  MilkCatHMMSegPOSTaggerProcessor(): term_instance_(NULL),
                                     part_of_speech_tag_instance_(NULL),
                                     hmm_segment_and_pos_tagger_(NULL) {
  }

  ~MilkCatHMMSegPOSTaggerProcessor() {
    if (term_instance_ != NULL) {
      delete term_instance_;
      term_instance_ = NULL;
    }

    if (part_of_speech_tag_instance_ != NULL) {
      delete part_of_speech_tag_instance_;
      part_of_speech_tag_instance_ = NULL;
    }

    if (hmm_segment_and_pos_tagger_ != NULL) {
      delete hmm_segment_and_pos_tagger_;
      hmm_segment_and_pos_tagger_ = NULL;
    }
  }

  int NextSentence() {
    if (0 == MilkCatProcessor::NextSentence())
      return 0;

    hmm_segment_and_pos_tagger_->Process(term_instance_, part_of_speech_tag_instance_, token_instance_);

    return 1;
  }

  size_t SentenceLength() { return term_instance_->size(); }
  const char *GetTerm(int position) { return term_instance_->term_text_at(position); }
  const char *GetPartOfSpeechTag(int position) { 
    return part_of_speech_tag_instance_->part_of_speech_tag_at(position); 
  }
  int GetWordType(int position) { return term_instance_->term_type_at(position); }

 protected:
  HMMSegmentAndPOSTagger *hmm_segment_and_pos_tagger_;
  TermInstance *term_instance_;
  PartOfSpeechTagInstance *part_of_speech_tag_instance_;
};

class HmmAndCrfProcessor: public MilkCatHMMSegPOSTaggerProcessor {
 public:
  static HmmAndCrfProcessor *Create(const char *model_dir_path) {
    HmmAndCrfProcessor *self = new HmmAndCrfProcessor();
    if (self->Initialize(model_dir_path) == false) {
      delete self;
      return NULL;
    } else {
      return self;
    }
  }

  bool Initialize(const char *model_dir_path) {
    if (MilkCatHMMSegPOSTaggerProcessor::Initialize(model_dir_path) == false)
      return false;

    next_term_instance_ = new TermInstance();
    next_part_of_speech_tag_instance_ = new PartOfSpeechTagInstance();

    out_of_vocabulary_word_recognitioin_ = OutOfVocabularyWordRecognition::Create(
        (std::string(model_dir_path) + "pd_model").c_str(), 
        (std::string(model_dir_path) + "ctb_pos.model").c_str(), 
        (std::string(model_dir_path) + "filter_word.darts").c_str());

    if (out_of_vocabulary_word_recognitioin_ == NULL) 
      return false;

    return true;
  } 

  HmmAndCrfProcessor(): next_term_instance_(NULL),
                        next_part_of_speech_tag_instance_(NULL),
                        out_of_vocabulary_word_recognitioin_(NULL) {
  }

  ~HmmAndCrfProcessor() {
    if (next_term_instance_ != NULL) {
      delete next_term_instance_;
      next_term_instance_ = NULL;
    }

    if (next_part_of_speech_tag_instance_ != NULL) {
      delete next_part_of_speech_tag_instance_;
      next_part_of_speech_tag_instance_ = NULL;
    }

    if (out_of_vocabulary_word_recognitioin_ != NULL) {
      delete out_of_vocabulary_word_recognitioin_;
      out_of_vocabulary_word_recognitioin_ = NULL;
    }
  }

  int NextSentence() {
    if (0 == MilkCatHMMSegPOSTaggerProcessor::NextSentence())
      return 0;

    out_of_vocabulary_word_recognitioin_->Process(
        next_term_instance_, 
        next_part_of_speech_tag_instance_, 
        term_instance_,
        part_of_speech_tag_instance_,
        token_instance_);

    return 1;
  }

  size_t SentenceLength() { return next_term_instance_->size(); }
  const char *GetTerm(int position) { return next_term_instance_->term_text_at(position); }
  const char *GetPartOfSpeechTag(int position) { 
    return next_part_of_speech_tag_instance_->part_of_speech_tag_at(position); 
  }
  int GetWordType(int position) { return next_term_instance_->term_type_at(position); }

 protected:
  TermInstance *next_term_instance_;
  PartOfSpeechTagInstance *next_part_of_speech_tag_instance_;
  OutOfVocabularyWordRecognition *out_of_vocabulary_word_recognitioin_;
};

class MilkCatCRFSegProcessor: public MilkCatProcessor {
 public:
  static MilkCatCRFSegProcessor *Create(const char *model_dir_path) {
    MilkCatCRFSegProcessor *self = new MilkCatCRFSegProcessor();
    if (self->Initialize(model_dir_path) == false) {
      delete self;
      return NULL;
    } else {
      return self;
    }
  }

  bool Initialize(const char *model_dir_path) {
    if (MilkCatProcessor::Initialize() == false) return false;
    std::string model_path = std::string(model_dir_path) + "pd_model";

    crf_segmenter_ = CRFSegmenter::Create(model_path.c_str());
    if (crf_segmenter_ == NULL) return false;

    term_instance_ = new TermInstance();
    return true;
  }

  MilkCatCRFSegProcessor(): crf_segmenter_(NULL),
                            term_instance_(NULL) {
  }

  ~MilkCatCRFSegProcessor() {
    if (crf_segmenter_ != NULL) {
      delete crf_segmenter_;
      crf_segmenter_ = NULL;
    }

    if (term_instance_ != NULL) {
      delete term_instance_;
      term_instance_ = NULL;
    }
  }
  
  int NextSentence() {
    if (MilkCatProcessor::NextSentence() == 0) return 0;
    crf_segmenter_->Segment(term_instance_, token_instance_);

    return 1;
  }

  size_t SentenceLength() { return term_instance_->size(); }
  const char *GetTerm(int position) { return term_instance_->term_text_at(position); }
  int GetWordType(int position) { return term_instance_->term_type_at(position); }

 protected:
  CRFSegmenter *crf_segmenter_;
  TermInstance *term_instance_;
  
};

class MilkCatCRFSegPOSTagProcessor: public MilkCatCRFSegProcessor {
 public:
  static MilkCatCRFSegPOSTagProcessor *Create(const char *model_dir_path) {
    MilkCatCRFSegPOSTagProcessor *self = new MilkCatCRFSegPOSTagProcessor();
    if (self->Initialize(model_dir_path) == false) {
      delete self;
      return NULL;
    } else {
      return self;
    }
  }

  bool Initialize(const char *model_dir_path) {
    std::string model_path = std::string(model_dir_path) + "ctb_pos.model";
    if (MilkCatCRFSegProcessor::Initialize(model_dir_path) == false) return false;

    part_of_speech_tag_instance_ = new PartOfSpeechTagInstance();
    crf_pos_tagger_ = CRFPOSTagger::Create(model_path.c_str());
    if (crf_pos_tagger_ == NULL)
      return false;

    return true;
  }

  ~MilkCatCRFSegPOSTagProcessor() {
    if (crf_pos_tagger_ != NULL) {
      delete crf_pos_tagger_;
      crf_pos_tagger_ = NULL;
    }

    if (part_of_speech_tag_instance_ != NULL) {
      delete part_of_speech_tag_instance_;
      part_of_speech_tag_instance_ = NULL;
    }
  }

  int NextSentence() {
    if (MilkCatCRFSegProcessor::NextSentence() == 0) return 0;
    crf_pos_tagger_->Tag(part_of_speech_tag_instance_, term_instance_);

    return 1;
  }

  const char *GetPartOfSpeechTag(int position) { 
    return part_of_speech_tag_instance_->part_of_speech_tag_at(position); 
  }

 protected:
  CRFPOSTagger *crf_pos_tagger_;
  PartOfSpeechTagInstance *part_of_speech_tag_instance_;
};


struct milkcat_t {
  MilkCatProcessor *processor;
  int sentence_length;
  int current_position;
};

milkcat_t *milkcat_init(int processor_type, const char *model_dir_path) {
  milkcat_t *milkcat = new milkcat_t;
  milkcat->sentence_length = 0;
  milkcat->current_position = 0;

  switch (processor_type) {
   case NORMAL_PROCESSOR:
    milkcat->processor = HmmAndCrfProcessor::Create(model_dir_path);
    break;

   case CRF_SEGMENTER:
    milkcat->processor = MilkCatCRFSegProcessor::Create(model_dir_path);
    break;

   case CRF_PROCESSOR:
    milkcat->processor = MilkCatCRFSegPOSTagProcessor::Create(model_dir_path);
    break;

   case HMM_PROCESSOR:
    milkcat->processor = MilkCatHMMSegPOSTaggerProcessor::Create(model_dir_path);
    break;

   default:
    milkcat->processor = NULL;
    break;
  }

  if (milkcat->processor == NULL) {
    delete milkcat;
    return NULL;  
  } else {
    return milkcat;
  }
}

void milkcat_delete(milkcat_t *milkcat) {
  delete milkcat->processor;
  delete milkcat;
}

void milkcat_process(milkcat_t *milkcat, const char *text) {
  milkcat->processor->Process(text);
  milkcat->sentence_length = 0;
  milkcat->current_position = 0;
}

int milkcat_next(milkcat_t *milkcat) {
  int has_next;
  if (milkcat->current_position >= milkcat->sentence_length - 1) {
    has_next = milkcat->processor->NextSentence();
    if (has_next == 0)
      return 0;
    milkcat->sentence_length = milkcat->processor->SentenceLength();
    milkcat->current_position = 0;
  } else {
    milkcat->current_position++;
  }
  
  // printf("goto %d (%d\n", milkcat->current_position, milkcat->sentence_length);
  return 1;
}

const char *milkcat_get_word(milkcat_t *milkcat) {
  return milkcat->processor->GetTerm(milkcat->current_position);
}

const char *milkcat_get_part_of_speech_tag(milkcat_t *milkcat) {
  return milkcat->processor->GetPartOfSpeechTag(milkcat->current_position);
}

MC_WORD_TYPE milkcat_get_word_type(milkcat_t *milkcat) {
  return static_cast<MC_WORD_TYPE>(milkcat->processor->GetWordType(milkcat->current_position));
}

const char *milkcat_get_error_message() {
  return get_error_message();
}