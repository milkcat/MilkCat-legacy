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
#include "term_instance.h"
#include "token_instance.h"
#include "part_of_speech_tag_instance.h"
#include "bigram_segmenter.h"
#include "out_of_vocabulary_word_recognitioin.h"
#include "hmm_part_of_speech_tagger.h"



class Processor {
 public:
  Processor(): tokenization_(NULL),
               token_instance_(NULL) {
  }

  virtual ~Processor() {
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

 protected:
  Tokenization *tokenization_;
  TokenInstance *token_instance_;
};


class CRFSegProcessor: public Processor {
 public:
  static CRFSegProcessor *Create(const char *model_dir_path) {
    CRFSegProcessor *self = new CRFSegProcessor();
    if (self->Initialize(model_dir_path) == false) {
      delete self;
      return NULL;
    } else {
      return self;
    }
  }

  bool Initialize(const char *model_dir_path) {
    if (Processor::Initialize() == false) return false;
    std::string model_path = std::string(model_dir_path) + "pd_model";

    crf_segmenter_ = CRFSegmenter::Create(model_path.c_str());
    if (crf_segmenter_ == NULL) return false;

    term_instance_ = new TermInstance();
    return true;
  }

  CRFSegProcessor(): crf_segmenter_(NULL),
                     term_instance_(NULL) {
  }

  ~CRFSegProcessor() {
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
    if (Processor::NextSentence() == 0) return 0;
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

class BigramSegProcessor: public Processor {
 public:
  static BigramSegProcessor *Create(const char *model_dir_path) {
    BigramSegProcessor *self = new BigramSegProcessor();
    if (self->Initialize(model_dir_path) == false) {
      delete self;
      return NULL;
    } else {
      return self;
    }
  }

  bool Initialize(const char *model_dir_path) {
    if (Processor::Initialize() == false) return false;

    next_term_instance_ = new TermInstance();

    std::string unigram_index_path = std::string(model_dir_path) + "unigram.idx";
    std::string unigram_data_path = std::string(model_dir_path) + "unigram.bin";
    std::string bigram_path = std::string(model_dir_path) + "bigram.bin";

    bigram_segmenter_ = BigramSegmenter::Create(unigram_index_path.c_str(),
                                                unigram_data_path.c_str(),
                                                bigram_path.c_str());
    if (bigram_segmenter_ == NULL) return false;

    out_of_vocabulary_word_recognitioin_ = OutOfVocabularyWordRecognition::Create(
        (std::string(model_dir_path) + "pd_model").c_str(), 
        (std::string(model_dir_path) + "oov_property.idx").c_str());

    if (out_of_vocabulary_word_recognitioin_ == NULL) return false;

    term_instance_ = new TermInstance();
    return true;
  }

  BigramSegProcessor(): bigram_segmenter_(NULL),
                        term_instance_(NULL),
                        next_term_instance_(NULL),
                        out_of_vocabulary_word_recognitioin_(NULL) {
  }

  ~BigramSegProcessor() {
    if (bigram_segmenter_ != NULL) {
      delete bigram_segmenter_;
      bigram_segmenter_ = NULL;
    }

    if (term_instance_ != NULL) {
      delete term_instance_;
      term_instance_ = NULL;
    }

    if (next_term_instance_ != NULL) {
      delete next_term_instance_;
      next_term_instance_ = NULL;
    }

    if (out_of_vocabulary_word_recognitioin_ != NULL) {
      delete out_of_vocabulary_word_recognitioin_;
      out_of_vocabulary_word_recognitioin_ = NULL;
    }
  }
  
  int NextSentence() {
    if (Processor::NextSentence() == 0) return 0;
    bigram_segmenter_->Process(term_instance_, token_instance_);
    out_of_vocabulary_word_recognitioin_->Process(next_term_instance_,
                                                  term_instance_,
                                                  token_instance_);
    return 1;
  }

  size_t SentenceLength() { return next_term_instance_->size(); }
  const char *GetTerm(int position) { return next_term_instance_->term_text_at(position); }
  int GetWordType(int position) { return next_term_instance_->term_type_at(position); }

 protected:
  BigramSegmenter *bigram_segmenter_;
  TermInstance *term_instance_;
  TermInstance *next_term_instance_;
  OutOfVocabularyWordRecognition *out_of_vocabulary_word_recognitioin_;
};

class CRFSegPOSTagProcessor: public CRFSegProcessor {
 public:
  static CRFSegPOSTagProcessor *Create(const char *model_dir_path) {
    CRFSegPOSTagProcessor *self = new CRFSegPOSTagProcessor();
    if (self->Initialize(model_dir_path) == false) {
      delete self;
      return NULL;
    } else {
      return self;
    }
  }

  bool Initialize(const char *model_dir_path) {
    std::string model_path = std::string(model_dir_path) + "ctb_pos.model";
    if (CRFSegProcessor::Initialize(model_dir_path) == false) return false;

    part_of_speech_tag_instance_ = new PartOfSpeechTagInstance();
    crf_pos_tagger_ = CRFPOSTagger::Create(model_path.c_str());
    if (crf_pos_tagger_ == NULL)
      return false;

    return true;
  }

  ~CRFSegPOSTagProcessor() {
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
    if (CRFSegProcessor::NextSentence() == 0) return 0;
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

class BigramSegCRFPartOfSpeechTagProcessor: public BigramSegProcessor {
 public:
  static BigramSegCRFPartOfSpeechTagProcessor *Create(const char *model_dir_path) {
    BigramSegCRFPartOfSpeechTagProcessor *self = new BigramSegCRFPartOfSpeechTagProcessor();
    if (self->Initialize(model_dir_path) == false) {
      delete self;
      return NULL;
    } else {
      return self;
    }
  }

  bool Initialize(const char *model_dir_path) {
    std::string model_path = std::string(model_dir_path) + "ctb_pos.model";
    if (BigramSegProcessor::Initialize(model_dir_path) == false) return false;

    part_of_speech_tag_instance_ = new PartOfSpeechTagInstance();
    crf_pos_tagger_ = CRFPOSTagger::Create(model_path.c_str());
    if (crf_pos_tagger_ == NULL)
      return false;

    return true;
  }

  ~BigramSegCRFPartOfSpeechTagProcessor() {
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
    if (BigramSegProcessor::NextSentence() == 0) return 0;
    crf_pos_tagger_->Tag(part_of_speech_tag_instance_, next_term_instance_);

    return 1;
  }

  const char *GetPartOfSpeechTag(int position) { 
    return part_of_speech_tag_instance_->part_of_speech_tag_at(position); 
  }

 protected:
  CRFPOSTagger *crf_pos_tagger_;
  PartOfSpeechTagInstance *part_of_speech_tag_instance_;
};

class BigramSegHMMPartOfSpeechTagProcessor: public BigramSegProcessor {
 public:
  static BigramSegHMMPartOfSpeechTagProcessor *Create(const char *model_dir_path) {
    BigramSegHMMPartOfSpeechTagProcessor *self = new BigramSegHMMPartOfSpeechTagProcessor();
    if (self->Initialize(model_dir_path) == false) {
      delete self;
      return NULL;
    } else {
      return self;
    }
  }

  bool Initialize(const char *model_dir_path) {
    std::string model_path = std::string(model_dir_path) + "ctb_pos.hmm";
    std::string index_path = std::string(model_dir_path) + "unigram.idx";
    if (BigramSegProcessor::Initialize(model_dir_path) == false) return false;

    part_of_speech_tag_instance_ = new PartOfSpeechTagInstance();
    hmm_pos_tagger_ = HMMPartOfSpeechTagger::Create(model_path.c_str(), index_path.c_str());
    if (hmm_pos_tagger_ == NULL)
      return false;

    return true;
  }

  ~BigramSegHMMPartOfSpeechTagProcessor() {
    if (hmm_pos_tagger_ != NULL) {
      delete hmm_pos_tagger_;
      hmm_pos_tagger_ = NULL;
    }

    if (part_of_speech_tag_instance_ != NULL) {
      delete part_of_speech_tag_instance_;
      part_of_speech_tag_instance_ = NULL;
    }
  }

  int NextSentence() {
    if (BigramSegProcessor::NextSentence() == 0) return 0;
    hmm_pos_tagger_->Tag(part_of_speech_tag_instance_, next_term_instance_);

    return 1;
  }

  const char *GetPartOfSpeechTag(int position) { 
    return part_of_speech_tag_instance_->part_of_speech_tag_at(position); 
  }

 protected:
  HMMPartOfSpeechTagger *hmm_pos_tagger_;
  PartOfSpeechTagInstance *part_of_speech_tag_instance_;
};

struct milkcat_t {
  Processor *processor;
  int sentence_length;
  int current_position;
};

milkcat_t *milkcat_init(int processor_type, const char *model_dir_path) {
  milkcat_t *milkcat = new milkcat_t;
  milkcat->sentence_length = 0;
  milkcat->current_position = 0;

  switch (processor_type) {
   case NORMAL_PROCESSOR:
    milkcat->processor = BigramSegHMMPartOfSpeechTagProcessor::Create(model_dir_path);
    break;

   case CRF_SEGMENTER:
    milkcat->processor = BigramSegProcessor::Create(model_dir_path);
    break;

   case CRF_PROCESSOR:
    milkcat->processor = CRFSegPOSTagProcessor::Create(model_dir_path);
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