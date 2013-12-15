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

struct milkcat_t {
  ModelFactory *model_factory;
  Status status;
};

struct milkcat_parser_t {
  Segmenter *segmenter;
  PartOfSpeechTagger *part_of_speech_tagger;
};

struct milkcat_cursor_t {
  milkcat_parser_t *parser;
  Tokenization *tokenizer;

  TokenInstance *token_instance;
  TermInstance *term_instance;
  PartOfSpeechTagInstance *part_of_speech_tag_instance;

  int sentence_length;
  int current_position;
};

milkcat_t *milkcat_new(const char *model_path) {
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

void milkcat_set_user_dictionary(milkcat_t *milkcat, const char *path) {
  milkcat->model_factory->SetUserDictionaryPath(path);
}

milkcat_parser_t *milkcat_parser_new(milkcat_t *milkcat, int parser_type) {
  milkcat_parser_t *parser = new milkcat_parser_t;

  switch (parser_type) {
   case DEFAULT_PROCESSOR:
    if (milkcat->status.ok()) parser->segmenter = SegmenterFactory(
        milkcat->model_factory, 
        kMixedSegmenter, 
        milkcat->status);
    if (milkcat->status.ok()) parser->part_of_speech_tagger = PartOfSpeechTaggerFactory(
        milkcat->model_factory,
        kMixedPartOfSpeechTagger, 
        milkcat->status);
    break;

   case CRF_SEGMENTER:
    if (milkcat->status.ok()) parser->segmenter = SegmenterFactory(
        milkcat->model_factory, 
        kCrfSegmenter, 
        milkcat->status);
    parser->part_of_speech_tagger = NULL;
    break;

   case CRF_PROCESSOR:
    if (milkcat->status.ok()) parser->segmenter = SegmenterFactory(
        milkcat->model_factory, 
        kCrfSegmenter, 
        milkcat->status);
    if (milkcat->status.ok()) parser->part_of_speech_tagger = PartOfSpeechTaggerFactory(
        milkcat->model_factory, 
        kCrfPartOfSpeechTagger, 
        milkcat->status);
    break;

   case DEFAULT_SEGMENTER:
    if (milkcat->status.ok()) parser->segmenter = SegmenterFactory(
        milkcat->model_factory, 
        kMixedSegmenter, 
        milkcat->status);
    parser->part_of_speech_tagger = NULL;
    break;    

   default:
    milkcat->status = Status::NotImplemented("");
    break;
  }

  if (!milkcat->status.ok()) {
    delete parser;
    return NULL;  
  } else {
    return parser;
  }
}

void milkcat_parser_destroy(milkcat_parser_t *parser) {
  delete parser->segmenter;
  parser->segmenter = NULL;

  delete parser->part_of_speech_tagger;
  parser->part_of_speech_tagger = NULL;

  delete parser;
}

milkcat_cursor_t *milkcat_cursor_new() {
  milkcat_cursor_t *cursor = new milkcat_cursor_t;
  cursor->parser = NULL;
  cursor->tokenizer = TokenizerFactory(kDefaultTokenizer);
  cursor->token_instance = new TokenInstance();
  cursor->term_instance = new TermInstance();
  cursor->part_of_speech_tag_instance = new PartOfSpeechTagInstance();
  cursor->current_position = 0;
  cursor->sentence_length = 0;
  return cursor;
}

void milkcat_cursor_destroy(milkcat_cursor_t *cursor) {
  delete cursor->tokenizer;
  delete cursor->token_instance;
  delete cursor->term_instance;
  delete cursor->part_of_speech_tag_instance;
  delete cursor;
}

void milkcat_parse(milkcat_parser_t *parser, milkcat_cursor_t *cursor, const char *text) {
  cursor->tokenizer->Scan(text);
  cursor->sentence_length = 0;
  cursor->current_position = 0;
  cursor->parser = parser;
}

int milkcat_cursor_next(milkcat_cursor_t *cursor) {
  
  if (cursor->current_position >= cursor->sentence_length - 1) {
    // If reached the end of current sentence

    if (cursor->tokenizer->GetSentence(cursor->token_instance) == false)
      return false;

    cursor->parser->segmenter->Segment(cursor->term_instance, cursor->token_instance);
    if (cursor->parser->part_of_speech_tagger) {
      cursor->parser->part_of_speech_tagger->Tag(
          cursor->part_of_speech_tag_instance, 
          cursor->term_instance);
    }

    cursor->sentence_length = cursor->term_instance->size();
    cursor->current_position = 0;
  } else {
    cursor->current_position++;
  }
  
  return true;
}

const char *milkcat_cursor_word(milkcat_cursor_t *cursor) {
  return cursor->term_instance->term_text_at(cursor->current_position);
}

const char *milkcat_cursor_pos_tag(milkcat_cursor_t *cursor) {
  if (cursor->parser->part_of_speech_tagger != NULL)
    return cursor->part_of_speech_tag_instance->part_of_speech_tag_at(cursor->current_position);
  else
    return NULL;
}

MC_WORD_TYPE milkcat_cursor_word_type(milkcat_cursor_t *cursor) {
  return static_cast<MC_WORD_TYPE>(cursor->term_instance->term_type_at(cursor->current_position));
}

const char *milkcat_get_error_message(milkcat_t *milkcat) {
  return milkcat->status.what();
}