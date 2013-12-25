//
// Yaca: Yet Another Chinese Segmenter, Part-Of-Speech Tagger and Dependency Parser
//
// libyaca.cc 
// Created at 2013-09-03
//


#include <string>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <config.h>
#include "milkcat.h"
#include "crf_tagger.h"
#include "tokenizer.h"
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

    delete user_index_;
    user_index_ = NULL;

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

  void SetUserDictionaryPath(const char *path) {
    user_dictionary_path_ = path;
  }

  bool HasUserDictionary() {
    return user_dictionary_path_.size() != 0;
  }

  const TrieTree *GetUserWordIndex(Status &status) {
    pthread_mutex_lock(&mutex);
    if (user_index_ == NULL) {
      user_index_ = DoubleArrayTrieTree::NewFromText(user_dictionary_path_.c_str(), status);
    }
    pthread_mutex_unlock(&mutex);
    return user_index_;
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
  std::string user_dictionary_path_;;
  pthread_mutex_t mutex;

  const TrieTree *unigram_index_;
  const TrieTree *user_index_;
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
  const TrieTree *index = NULL;
  const TrieTree *user_index = NULL;
  const StaticArray<float> *unigram_cost = NULL;
  const StaticHashTable<int64_t, float> *bigram_cost = NULL;
  const CRFModel *seg_model = NULL;
  const TrieTree *oov_property = NULL;

  switch (segmenter_id) {
   case kBigramSegmenter:
    if (status.ok()) index = factory->GetWordIndex(status);
    if (status.ok() && factory->HasUserDictionary()) user_index = factory->GetUserWordIndex(status);
    if (status.ok()) unigram_cost = factory->GetUnigramCostData(status);
    if (status.ok()) bigram_cost = factory->GetBigramCostData(status);

    if (status.ok()) {
      return new BigramSegmenter(index, user_index, unigram_cost, bigram_cost);
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
    if (status.ok() && factory->HasUserDictionary()) user_index = factory->GetUserWordIndex(status);
    if (status.ok()) unigram_cost = factory->GetUnigramCostData(status);
    if (status.ok()) bigram_cost = factory->GetBigramCostData(status);
    if (status.ok()) seg_model = factory->GetCRFSegModel(status);
    if (status.ok()) oov_property = factory->GetOOVProperty(status);

    if (status.ok()) {
      return MixedSegmenter::New(
        index,
        user_index,
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

Status global_status;

struct milkcat_cursor_t;

struct milkcat_t {
  ModelFactory *model_factory;
  bool owned_model_factory;

  Segmenter *segmenter;
  PartOfSpeechTagger *part_of_speech_tagger;
  
  std::vector<milkcat_cursor_t *> cursor_pool;
};

struct milkcat_cursor_t {
  milkcat_t *milkcat;

  Tokenization *tokenizer;
  TokenInstance *token_instance;
  TermInstance *term_instance;
  PartOfSpeechTagInstance *part_of_speech_tag_instance;

  int sentence_length;
  int current_position;
  bool end;
};

void CursorMoveToNext(milkcat_cursor_t *c) {
  c->current_position++;
  if (c->current_position > c->sentence_length - 1) {
    // If reached the end of current sentence

    if (c->tokenizer->GetSentence(c->token_instance) == false) {
      c->end = true;
      return ;
    }

    c->milkcat->segmenter->Segment(c->term_instance, c->token_instance);
    if (c->milkcat->part_of_speech_tagger) {
      c->milkcat->part_of_speech_tagger->Tag(c->part_of_speech_tag_instance, c->term_instance);
    }
    
    c->sentence_length = c->term_instance->size();
    c->current_position = 0;
  } 
}

milkcat_t *milkcat_new(const char *model_path, int analyzer_type) {
  // Clear the global status's failed state
  global_status = Status::OK();

  if (model_path == NULL) model_path = MODEL_PATH;
  milkcat_t *m = new milkcat_t;
  m->model_factory = new ModelFactory(model_path);
  m->owned_model_factory = true;

  switch (analyzer_type) {
   case DEFAULT_PROCESSOR:
    if (global_status.ok()) m->segmenter = SegmenterFactory(
        m->model_factory, 
        kMixedSegmenter, 
        global_status);
    if (global_status.ok()) m->part_of_speech_tagger = PartOfSpeechTaggerFactory(
        m->model_factory,
        kMixedPartOfSpeechTagger, 
        global_status);
    break;

   case CRF_SEGMENTER:
    if (global_status.ok()) m->segmenter = SegmenterFactory(
        m->model_factory, 
        kCrfSegmenter, 
        global_status);
    m->part_of_speech_tagger = NULL;
    break;

   case CRF_PROCESSOR:
    if (global_status.ok()) m->segmenter = SegmenterFactory(
        m->model_factory, 
        kCrfSegmenter, 
        global_status);
    if (global_status.ok()) m->part_of_speech_tagger = PartOfSpeechTaggerFactory(
        m->model_factory, 
        kCrfPartOfSpeechTagger, 
        global_status);
    break;

   case DEFAULT_SEGMENTER:
    if (global_status.ok()) m->segmenter = SegmenterFactory(
        m->model_factory, 
        kMixedSegmenter, 
        global_status);
    m->part_of_speech_tagger = NULL;
    break;    

   default:
    global_status = Status::NotImplemented("");
    break;
  }

  if (!global_status.ok()) {
    milkcat_destroy(m);
    return NULL;  
  } else {
    return m;
  }
}

void milkcat_destory_cursor(milkcat_cursor_t *c) {
  delete c->tokenizer;
  c->tokenizer = NULL;

  delete c->token_instance;
  c->token_instance = NULL;

  delete c->term_instance;
  c->term_instance = NULL;

  delete c->part_of_speech_tag_instance;
  c->part_of_speech_tag_instance = NULL;

  delete c;
}

void milkcat_destroy(milkcat_t *m) {
  if (m->owned_model_factory) {
    delete m->model_factory;
  }
  m->model_factory = NULL;

  delete m->segmenter;
  m->segmenter = NULL;

  delete m->part_of_speech_tagger;
  m->part_of_speech_tagger = NULL;

  // Clear the cursor pool
  for (std::vector<milkcat_cursor_t *>::iterator it = m->cursor_pool.begin(); it != m->cursor_pool.end(); ++it) 
    milkcat_destory_cursor(*it);

  delete m;
}

void milkcat_set_userdict(milkcat_t *m, const char *path) {
  m->model_factory->SetUserDictionaryPath(path);
}


milkcat_cursor_t *milkcat_process(milkcat_t *m, const char *text) {
  milkcat_cursor_t *cursor;
  if (m->cursor_pool.empty()) {
    cursor = new milkcat_cursor_t;
    cursor->milkcat = m;
    cursor->tokenizer = TokenizerFactory(kDefaultTokenizer);
    cursor->token_instance = new TokenInstance();
    cursor->term_instance = new TermInstance();
    cursor->part_of_speech_tag_instance = new PartOfSpeechTagInstance();
    cursor->end = false;
  } else {
    cursor = m->cursor_pool.back();
    m->cursor_pool.pop_back();
  }

  cursor->tokenizer->Scan(text);
  cursor->sentence_length = 0;
  cursor->current_position = 0;

  CursorMoveToNext(cursor);
  return cursor;
}

void milkcat_cursor_release(milkcat_cursor_t *c) {
  c->milkcat->cursor_pool.push_back(c);
}

milkcat_item_t milkcat_cursor_get_next(milkcat_cursor_t *c) {
  milkcat_item_t item;

  item.word = c->term_instance->term_text_at(c->current_position);
  if (c->milkcat->part_of_speech_tagger != NULL)
    item.part_of_speech_tag = c->part_of_speech_tag_instance->part_of_speech_tag_at(c->current_position);
  else
    item.part_of_speech_tag = NULL;
  item.word_type = static_cast<MC_WORD_TYPE>(c->term_instance->term_type_at(c->current_position));

  CursorMoveToNext(c);
  return item;
}

bool milkcat_cursor_has_next(milkcat_cursor_t *c) {
  return c->end != true;
}

const char *milkcat_last_error() {
  return global_status.what();
}