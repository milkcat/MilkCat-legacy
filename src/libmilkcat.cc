//
// Yaca: Yet Another Chinese Segmenter, Part-Of-Speech Tagger and Dependency Parser
//
// libyaca.cc 
// Created at 2013-09-03
//


#include <string>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
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

// A factory class that can obtain any model data class needed by MilkCat
// in singleton mode. All the getXX fucnctions are thread safe
class ModelFactory {
 public:
  ModelFactory(const char *model_dir_path): 
      model_dir_path_(model_dir_path),
      unigram_index_(NULL),
      unigram_cost_(NULL),
      bigram_cost_(NULL),
      seg_model_(NULL),
      crf_pos_model_(NULL),
      hmm_pos_model_(NULL),
      oov_property_(NULL),
      default_tag_(NULL) {

    pthread_mutex_init(&mutex, NULL);
  }

  ~ModelFactory() {
    pthread_mutex_destroy(&mutex);

    delete unigram_index_;
    unigram_index_ = NULL;

    delete unigram_cost_;
    unigram_cost_ = NULL;

    delete bigram_cost_;
    bigram_cost_ = NULL;

    delete seg_model_;
    seg_model_ = NULL;

    delete crf_pos_model_;
    crf_pos_model_ = NULL;

    delete hmm_pos_model_;
    hmm_pos_model_ = NULL;

    delete oov_property_;
    oov_property_ = NULL;

    delete default_tag_;
    default_tag_ = NULL;    
  }

  // Get the index for word which were used in unigram cost, bigram cost
  // hmm pos model and oov property
  const TrieTree *GetWordIndex(Status &status) {
    pthread_mutex_lock(&mutex);
    if (unigram_index_ == NULL) {
      unigram_index_ = DoubleArrayTrieTree::New((model_dir_path_ + UNIGRAM_INDEX).c_str(), status);
    }
    pthread_mutex_unlock(&mutex);
    return unigram_index_;
  }

  const StaticArray<float> *GetUnigramCostData(Status &status) {
    pthread_mutex_lock(&mutex);
    if (unigram_cost_ == NULL) {
      unigram_cost_ = StaticArray<float>::New((model_dir_path_ + UNIGRAM_DATA).c_str(), status);
    }
    pthread_mutex_unlock(&mutex);
    return unigram_cost_;
  }

  const StaticHashTable<int64_t, float> *GetBigramCostData(Status &status) {
    pthread_mutex_lock(&mutex);
    if (bigram_cost_ == NULL) {
      bigram_cost_ = StaticHashTable<int64_t, float>::New((model_dir_path_ + BIGRAM_DATA).c_str(), status);
    }
    pthread_mutex_unlock(&mutex);
    return bigram_cost_;
  }

  // Get the CRF word segmenter model
  const CRFModel *GetCRFSegModel(Status &status) {
    pthread_mutex_lock(&mutex);
    if (seg_model_ == NULL) {
      seg_model_ = CRFModel::New((model_dir_path_ + CRF_SEGMENTER_MODEL).c_str(), status);
    }
    pthread_mutex_unlock(&mutex);
    return seg_model_;   
  }

  // Get the CRF word part-of-speech model
  const CRFModel *GetCRFPosModel(Status &status) {
    pthread_mutex_lock(&mutex);
    if (crf_pos_model_ == NULL) {
      crf_pos_model_ = CRFModel::New((model_dir_path_ + CRF_PART_OF_SPEECH_MODEL).c_str(), status);
    }
    pthread_mutex_unlock(&mutex);
    return crf_pos_model_;   
  }

  // Get the HMM word part-of-speech model
  const HMMModel *GetHMMPosModel(Status &status) {
    pthread_mutex_lock(&mutex);
    if (hmm_pos_model_ == NULL) {
      hmm_pos_model_ = HMMModel::New((model_dir_path_ + HMM_PART_OF_SPEECH_MODEL).c_str(), status);
    }
    pthread_mutex_unlock(&mutex);
    return hmm_pos_model_;   
  }

  // Get the character's property in out-of-vocabulary word recognition
  const TrieTree *GetOOVProperty(Status &status) {
    pthread_mutex_lock(&mutex);
    if (oov_property_ == NULL) {
      oov_property_ = DoubleArrayTrieTree::New((model_dir_path_ + OOV_PROPERTY).c_str(), status);
    }
    pthread_mutex_unlock(&mutex);
    return oov_property_;
  }

  const Configuration *GetDefaultTagConf(Status &status) {
    pthread_mutex_lock(&mutex);
    if (default_tag_ == NULL) {
      default_tag_ = Configuration::New((model_dir_path_ + DEFAULT_TAG).c_str(), status);
    }
    pthread_mutex_unlock(&mutex);
    return default_tag_;
  }

 private:
  std::string model_dir_path_;
  pthread_mutex_t mutex;

  const TrieTree *unigram_index_;
  const StaticArray<float> *unigram_cost_;
  const StaticHashTable<int64_t, float> *bigram_cost_;
  const CRFModel *seg_model_;
  const CRFModel *crf_pos_model_;
  const HMMModel *hmm_pos_model_;
  const TrieTree *oov_property_;
  const Configuration *default_tag_;
};

Tokenization *TokenizerFactory(int tokenizer_id) {
  switch (tokenizer_id) {
   case kDefaultTokenizer:
    return new Tokenization();

   default:
    return NULL;
  }
}

Segmenter *SegmenterFactory(ModelFactory *factory, int segmenter_id, Status &status) {
  const TrieTree *index;
  const StaticArray<float> *unigram_cost;
  const StaticHashTable<int64_t, float> *bigram_cost;
  const CRFModel *seg_model;
  const TrieTree *oov_property;

  switch (segmenter_id) {
   case kBigramSegmenter:
    if (status.ok()) index = factory->GetWordIndex(status);
    if (status.ok()) unigram_cost = factory->GetUnigramCostData(status);
    if (status.ok()) bigram_cost = factory->GetBigramCostData(status);

    if (status.ok()) {
      return new BigramSegmenter(index, unigram_cost, bigram_cost);
    } else {
      return NULL;
    }

   case kCrfSegmenter:
    if (status.ok()) seg_model = factory->GetCRFSegModel(status);

    if (status.ok()) {
      return CRFSegmenter::New(seg_model, status);
    } else {
      return NULL;
    }

   case kMixedSegmenter:
    if (status.ok()) index = factory->GetWordIndex(status);
    if (status.ok()) unigram_cost = factory->GetUnigramCostData(status);
    if (status.ok()) bigram_cost = factory->GetBigramCostData(status);
    if (status.ok()) seg_model = factory->GetCRFSegModel(status);
    if (status.ok()) oov_property = factory->GetOOVProperty(status);

    if (status.ok()) {
      return MixedSegmenter::New(
        index,
        unigram_cost,
        bigram_cost,
        seg_model,
        oov_property,
        status);      
    } else {
      return NULL;
    }


   default:
    status = Status::NotImplemented("");
    return NULL;
  }
}

PartOfSpeechTagger *PartOfSpeechTaggerFactory(ModelFactory *factory,
                                              int part_of_speech_tagger_id, 
                                              Status &status) {
  const HMMModel *hmm_pos_model;
  const CRFModel *crf_pos_model;
  const TrieTree *index;
  const Configuration *default_tag;

  switch (part_of_speech_tagger_id) {
   case kCrfPartOfSpeechTagger:
    if (status.ok()) crf_pos_model = factory->GetCRFPosModel(status);

    if (status.ok()) {
      return new CRFPartOfSpeechTagger(crf_pos_model);
    } else {
      return NULL;
    }

   case kHmmPartOfSpeechTagger:
    if (status.ok()) hmm_pos_model = factory->GetHMMPosModel(status);
    if (status.ok()) index = factory->GetWordIndex(status);
    if (status.ok()) default_tag = factory->GetDefaultTagConf(status);

    if (status.ok()) {
      return HMMPartOfSpeechTagger::New(hmm_pos_model, index, default_tag, status);  
    } else {
      return NULL;
    }

   case kMixedPartOfSpeechTagger:
    if (status.ok()) crf_pos_model = factory->GetCRFPosModel(status);
    if (status.ok()) hmm_pos_model = factory->GetHMMPosModel(status);
    if (status.ok()) index = factory->GetWordIndex(status);
    if (status.ok()) default_tag = factory->GetDefaultTagConf(status);

    if (status.ok()) {
      return MixedPartOfSpeechTagger::New(
        hmm_pos_model,
        index,
        default_tag,
        crf_pos_model,
        status);      
    } else {
      return NULL;
    }


   default:
    status = Status::NotImplemented("");
    return NULL;   
  }

}

struct milkcat_t {
  ModelFactory *model_factory;
  Status status;
};

struct milkcat_processor_t {
  Processor *processor;
  Tokenization *tokenizer;
  Segmenter *segmenetr;
  PartOfSpeechTagger *part_of_speech_tagger;

  int sentence_length;
  int current_position;
};

milkcat_t *milkcat_init(const char *model_path) {
  if (model_path == NULL) model_path = MODEL_PATH;
  milkcat_t *m = new milkcat_t;
  m->model_factory = new ModelFactory(model_path);
  return m;
}

void milkcat_destroy(milkcat_t *milkcat) {
  delete milkcat->model_factory;
  milkcat->model_factory = NULL;

  delete milkcat;
}

milkcat_processor_t *milkcat_processor_new(milkcat_t *milkcat, int processor_type) {
  milkcat_processor_t *proc = new milkcat_processor_t;
  proc->sentence_length = 0;
  proc->current_position = 0;

  

  switch (processor_type) {
   case DEFAULT_PROCESSOR:
    proc->tokenizer = TokenizerFactory(kDefaultTokenizer);
    if (milkcat->status.ok()) proc->segmenetr = SegmenterFactory(
        milkcat->model_factory, 
        kMixedSegmenter, 
        milkcat->status);
    if (milkcat->status.ok()) proc->part_of_speech_tagger = PartOfSpeechTaggerFactory(
        milkcat->model_factory,
        kMixedPartOfSpeechTagger, 
        milkcat->status);
    if (milkcat->status.ok())
      proc->processor = new Processor(proc->tokenizer, proc->segmenetr, proc->part_of_speech_tagger);
    else 
      proc->processor = NULL;
    break;

   case CRF_SEGMENTER:
    proc->tokenizer = TokenizerFactory(kDefaultTokenizer);
    if (milkcat->status.ok()) proc->segmenetr = SegmenterFactory(
        milkcat->model_factory, 
        kCrfSegmenter, 
        milkcat->status);
    proc->part_of_speech_tagger = NULL;
    if (milkcat->status.ok())
      proc->processor = new Processor(proc->tokenizer, proc->segmenetr, proc->part_of_speech_tagger);
    else 
      proc->processor = NULL;
    break;

   case CRF_PROCESSOR:
    proc->tokenizer = TokenizerFactory(kDefaultTokenizer);
    if (milkcat->status.ok()) proc->segmenetr = SegmenterFactory(
        milkcat->model_factory, 
        kCrfSegmenter, 
        milkcat->status);
    if (milkcat->status.ok()) proc->part_of_speech_tagger = PartOfSpeechTaggerFactory(
        milkcat->model_factory, 
        kCrfPartOfSpeechTagger, 
        milkcat->status);
    if (milkcat->status.ok())
      proc->processor = new Processor(proc->tokenizer, proc->segmenetr, proc->part_of_speech_tagger);
    else 
      proc->processor = NULL;
    break;

   case DEFAULT_SEGMENTER:
    proc->tokenizer = TokenizerFactory(kDefaultTokenizer);
    if (milkcat->status.ok()) proc->segmenetr = SegmenterFactory(
        milkcat->model_factory, 
        kMixedSegmenter, 
        milkcat->status);
    proc->part_of_speech_tagger = NULL;
    if (milkcat->status.ok())
      proc->processor = new Processor(proc->tokenizer, proc->segmenetr, proc->part_of_speech_tagger);
    else 
      proc->processor = NULL;
    break;    

   default:
    proc->processor = NULL;
    break;
  }

  if (proc->processor == NULL) {
    delete proc;
    return NULL;  
  } else {
    return proc;
  }
}

void milkcat_processor_delete(milkcat_processor_t *proc) {
  delete proc->processor;
  proc->processor = NULL;

  delete proc->tokenizer;
  proc->tokenizer = NULL;

  delete proc->segmenetr;
  proc->segmenetr = NULL;

  delete proc->part_of_speech_tagger;
  proc->part_of_speech_tagger = NULL;

  delete proc;
}

void milkcat_process(milkcat_processor_t *proc, const char *text) {
  proc->processor->Process(text);
  proc->sentence_length = 0;
  proc->current_position = 0;
}

int milkcat_next(milkcat_processor_t *proc) {
  int has_next;
  if (proc->current_position >= proc->sentence_length - 1) {
    has_next = proc->processor->NextSentence();
    if (has_next == 0)
      return 0;
    proc->sentence_length = proc->processor->SentenceLength();
    proc->current_position = 0;
  } else {
    proc->current_position++;
  }
  
  // printf("goto %d (%d\n", milkcat->current_position, milkcat->sentence_length);
  return 1;
}

const char *milkcat_get_word(milkcat_processor_t *proc) {
  return proc->processor->GetTerm(proc->current_position);
}

const char *milkcat_get_part_of_speech_tag(milkcat_processor_t *proc) {
  return proc->processor->GetPartOfSpeechTag(proc->current_position);
}

MC_WORD_TYPE milkcat_get_word_type(milkcat_processor_t *proc) {
  return static_cast<MC_WORD_TYPE>(proc->processor->GetWordType(proc->current_position));
}

const char *milkcat_get_error_message(milkcat_t *milkcat) {
  return milkcat->status.what();
}