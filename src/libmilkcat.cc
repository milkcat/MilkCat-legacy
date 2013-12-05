//
// Yaca: Yet Another Chinese Segmenter, Part-Of-Speech Tagger and Dependency Parser
//
// libyaca.cc 
// Created at 2013-09-03
//


#include <string>
#include <string.h>
#include <stdio.h>
#include <config.h>
#include "milkcat.h"
#include "crf_tagger.h"
#include "tokenization.h"
#include "utils.h"
#include "term_instance.h"
#include "token_instance.h"
#include "part_of_speech_tag_instance.h"
#include "bigram_segmenter.h"
#include "out_of_vocabulary_word_recognition.h"
#include "mixed_segmenter.h"
#include "hmm_part_of_speech_tagger.h"
#include "mixed_part_of_speech_tagger.h"
#include "crf_part_of_speech_tagger.h"
#include "hmm_model.h"

const int kDefaultTokenizer = 0;

const int kBigramSegmenter = 0;
const int kCrfSegmenter = 1;
const int kMixedSegmenter = 2;

const int kCrfPartOfSpeechTagger = 0;
const int kHmmPartOfSpeechTagger = 1;
const int kMixedPartOfSpeechTagger = 2;

const char *UNIGRAM_INDEX = "unigram.idx";
const char *UNIGRAM_DATA = "unigram.bin";
const char *BIGRAM_DATA = "bigram.bin";
const char *HMM_PART_OF_SPEECH_MODEL = "ctb_pos.hmm";
const char *CRF_PART_OF_SPEECH_MODEL = "ctb_pos.crf";
const char *CRF_SEGMENTER_MODEL = "ctb_seg.crf";
const char *DEFAULT_TAG = "default_tag.cfg";
const char *OOV_PROPERTY = "oov_property.idx";

class Processor {
 public:
  Processor(Tokenization *tokenizer, Segmenter *segmenetr, PartOfSpeechTagger *part_of_speech_tagger):
      tokenizer_(tokenizer), 
      segmenter_(segmenetr),
      part_of_speech_tagger_(part_of_speech_tagger) {
        
    token_instance_ = new TokenInstance();
    term_instance_ = new TermInstance();
    part_of_speech_tag_instance_ = new PartOfSpeechTagInstance();
  }

  ~Processor() {
    delete token_instance_;
    token_instance_ = NULL;

    delete term_instance_;
    term_instance_ = NULL;

    delete part_of_speech_tag_instance_;
    part_of_speech_tag_instance_ = NULL;
  }

  // Parse the next sentence return 1 if has next sentence else return 0
  int NextSentence() {
    if (tokenizer_->GetSentence(token_instance_) == false) return 0;
    segmenter_->Segment(term_instance_, token_instance_);

    if (part_of_speech_tagger_ != NULL) 
      part_of_speech_tagger_->Tag(part_of_speech_tag_instance_, term_instance_);

    return 1;
  }

  // Start to process a sentence
  void Process(const char *text) { 
    tokenizer_->Scan(text); 
  }

  // Get term/part-of-speech tag number of a sentence
  int SentenceLength() { 
    return term_instance_->size();
  }

  // Get the term at position of current senetnce
  const char *GetTerm(int position) { 
    return term_instance_->term_text_at(position); 
  }

  // Get the part-of-speech tag of term in current sentence
  // If no part-of-speech tagger return NULL
  const char *GetPartOfSpeechTag(int position) { 
    if (part_of_speech_tagger_ == NULL) return NULL;
    return part_of_speech_tag_instance_->part_of_speech_tag_at(position); 
  }

  // Get the term type in current sentence 
  int GetWordType(int position) { 
    return term_instance_->term_type_at(position); 
  }

 protected:
  Tokenization *tokenizer_;
  Segmenter *segmenter_;
  PartOfSpeechTagger *part_of_speech_tagger_;

  TokenInstance *token_instance_;
  TermInstance *term_instance_;
  PartOfSpeechTagInstance *part_of_speech_tag_instance_;
};

Tokenization *TokenizerFactory(int tokenizer_id) {
  switch (tokenizer_id) {
   case kDefaultTokenizer:
    return new Tokenization();

   default:
    set_error_message("invalid tokenizer_id.");
    return NULL;
  }
}

Segmenter *SegmenterFactory(int segmenter_id, const char *model_path) {
  std::string model_path_str(model_path);

  std::string unigram_index = model_path_str + UNIGRAM_INDEX;
  std::string unigram_data = model_path_str + UNIGRAM_DATA;
  std::string bigram_data = model_path_str + BIGRAM_DATA;
  std::string crf_seg_model = model_path_str + CRF_SEGMENTER_MODEL;
  std::string oov_property = model_path_str + OOV_PROPERTY;

  switch (segmenter_id) {
   case kBigramSegmenter:
    return BigramSegmenter::Create(
      unigram_index.c_str(),
      unigram_data.c_str(),
      bigram_data.c_str());

   case kCrfSegmenter:
    return CRFSegmenter::Create(crf_seg_model.c_str());

   case kMixedSegmenter:
    return MixedSegmenter::Create(
      unigram_index.c_str(),
      unigram_data.c_str(),
      bigram_data.c_str(),
      oov_property.c_str(),
      crf_seg_model.c_str());

   default:
    set_error_message("invalid segmenter_id.");
    return NULL;
  }
}

PartOfSpeechTagger *PartOfSpeechTaggerFactory(int part_of_speech_tagger_id, const char *model_path) {
  std::string model_path_str(model_path);

  std::string unigram_index = model_path_str + UNIGRAM_INDEX;
  std::string crf_pos_model = model_path_str + CRF_PART_OF_SPEECH_MODEL;
  std::string hmm_pos_model = model_path_str + HMM_PART_OF_SPEECH_MODEL;
  std::string default_tag = model_path_str + DEFAULT_TAG;

  switch (part_of_speech_tagger_id) {
   case kCrfPartOfSpeechTagger:
    return CRFPartOfSpeechTagger::Create(crf_pos_model.c_str());

   case kHmmPartOfSpeechTagger:
    return HMMPartOfSpeechTagger::Create(
      hmm_pos_model.c_str(),
      unigram_index.c_str(),
      default_tag.c_str());

   case kMixedPartOfSpeechTagger:
    return MixedPartOfSpeechTagger::Create(
      hmm_pos_model.c_str(),
      unigram_index.c_str(),
      default_tag.c_str(),
      crf_pos_model.c_str());

   default:
    set_error_message("invalid part_of_speech_tagger_id.");
    return NULL;   
  }

}

struct milkcat_t {
  Processor *processor;
  Tokenization *tokenizer;
  Segmenter *segmenetr;
  PartOfSpeechTagger *part_of_speech_tagger;

  int sentence_length;
  int current_position;
};

milkcat_t *milkcat_init(int processor_type, const char *model_dir_path) {
  milkcat_t *milkcat = new milkcat_t;
  milkcat->sentence_length = 0;
  milkcat->current_position = 0;

  if (model_dir_path == NULL) model_dir_path = MODEL_PATH;

  switch (processor_type) {
   case DEFAULT_PROCESSOR:
    milkcat->tokenizer = TokenizerFactory(kDefaultTokenizer);
    milkcat->segmenetr = SegmenterFactory(kMixedSegmenter, model_dir_path);
    milkcat->part_of_speech_tagger = PartOfSpeechTaggerFactory(kMixedPartOfSpeechTagger, model_dir_path);
    if (milkcat->tokenizer != NULL && milkcat->segmenetr != NULL && milkcat->part_of_speech_tagger != NULL)
      milkcat->processor = new Processor(milkcat->tokenizer, milkcat->segmenetr, milkcat->part_of_speech_tagger);
    else 
      milkcat->processor = NULL;
    break;

   case CRF_SEGMENTER:
    milkcat->tokenizer = TokenizerFactory(kDefaultTokenizer);
    milkcat->segmenetr = SegmenterFactory(kCrfSegmenter, model_dir_path);
    milkcat->part_of_speech_tagger = NULL;
    if (milkcat->tokenizer != NULL && milkcat->segmenetr != NULL)
      milkcat->processor = new Processor(milkcat->tokenizer, milkcat->segmenetr, milkcat->part_of_speech_tagger);
    else 
      milkcat->processor = NULL;
    break;

   case CRF_PROCESSOR:
    milkcat->tokenizer = TokenizerFactory(kDefaultTokenizer);
    milkcat->segmenetr = SegmenterFactory(kCrfSegmenter, model_dir_path);
    milkcat->part_of_speech_tagger = PartOfSpeechTaggerFactory(kCrfPartOfSpeechTagger, model_dir_path);
    if (milkcat->tokenizer != NULL && milkcat->segmenetr != NULL && milkcat->part_of_speech_tagger != NULL)
      milkcat->processor = new Processor(milkcat->tokenizer, milkcat->segmenetr, milkcat->part_of_speech_tagger);
    else 
      milkcat->processor = NULL;
    break;

   case DEFAULT_SEGMENTER:
    milkcat->tokenizer = TokenizerFactory(kDefaultTokenizer);
    milkcat->segmenetr = SegmenterFactory(kMixedSegmenter, model_dir_path);
    milkcat->part_of_speech_tagger = NULL;
    if (milkcat->tokenizer != NULL && milkcat->segmenetr != NULL)
      milkcat->processor = new Processor(milkcat->tokenizer, milkcat->segmenetr, milkcat->part_of_speech_tagger);
    else 
      milkcat->processor = NULL;
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
  milkcat->processor = NULL;

  delete milkcat->tokenizer;
  milkcat->tokenizer = NULL;

  delete milkcat->segmenetr;
  milkcat->segmenetr = NULL;

  delete milkcat->part_of_speech_tagger;
  milkcat->part_of_speech_tagger = NULL;

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