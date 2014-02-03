//
// libyaca.cc
// libmilkcat.h --- Created at 2013-09-03
//
// The MIT License (MIT)
//
// Copyright (c) 2013 ling0322 <ling032x@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//


#include <string>
#include <vector>
#include <string.h>
#include <stdio.h>
#include "utils/utils.h"
#include "milkcat.h"
#include "crf_tagger.h"
#include "tokenizer.h"
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
      user_index_(NULL),
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


// The global state saves the current of model and analyzer.
// If any errors occured, global_status != Status::OK()
Status global_status;

class Cursor;

struct milkcat_t {
  milkcat_model_t *model;

  Segmenter *segmenter;
  PartOfSpeechTagger *part_of_speech_tagger;
  
  std::vector<Cursor *> cursor_pool;
};

// Cursor class save the internal state of the analyzing result, such as
// the current word and current sentence. 
class Cursor {
 public:
  Cursor(milkcat_t *analyzer):
      analyzer_(analyzer),
      tokenizer_(TokenizerFactory(kDefaultTokenizer)),
      token_instance_(new TokenInstance()),
      term_instance_(new TermInstance()),
      part_of_speech_tag_instance_(new PartOfSpeechTagInstance()),
      sentence_length_(0),
      current_position_(0),
      end_(0) {  
  }

  ~Cursor() {
    delete tokenizer_;
    tokenizer_ = NULL;

    delete token_instance_;
    token_instance_ = NULL;

    delete term_instance_;
    term_instance_ = NULL;

    delete part_of_speech_tag_instance_;
    part_of_speech_tag_instance_ = NULL;

    analyzer_ = NULL;
  }

  // Start to scan a text and use this->analyzer_ to analyze it
  // the result saved in its current state, use MoveToNext() to
  // iterate.
  void Scan(const char *text) {
    tokenizer_->Scan(text);
    sentence_length_ = 0;
    current_position_ = 0;
    end_ = false;
  }

  // Move the cursor to next position, if end of text is reached
  // set end() to true
  void MoveToNext() {
    current_position_++;
    if (current_position_ > sentence_length_ - 1) {
      // If reached the end of current sentence

      if (tokenizer_->GetSentence(token_instance_) == false) {
        end_ = true;
      } else {
        analyzer_->segmenter->Segment(term_instance_, token_instance_);

        // If the analyzer have part of speech tagger, tag the term_instance
        if (analyzer_->part_of_speech_tagger) {
          analyzer_->part_of_speech_tagger->Tag(part_of_speech_tag_instance_, term_instance_);
        }
        sentence_length_ = term_instance_->size();
        current_position_ = 0;
      }
    } 
  }

  const char *word() const { return term_instance_->term_text_at(current_position_); }

  const char *part_of_speech_tag() const {
    if (analyzer_->part_of_speech_tagger != NULL)
      return part_of_speech_tag_instance_->part_of_speech_tag_at(current_position_);
    else
      return "NONE";
  }

  const int word_type() const {
    return term_instance_->term_type_at(current_position_);
  }

  // If reaches the end of text
  bool end() const { return end_; }

  milkcat_t *analyzer() const { return analyzer_; }
  
 private:
  milkcat_t *analyzer_;

  Tokenization *tokenizer_;
  TokenInstance *token_instance_;
  TermInstance *term_instance_;
  PartOfSpeechTagInstance *part_of_speech_tag_instance_;

  int sentence_length_;
  int current_position_;
  bool end_;
};

struct milkcat_model_t {
  ModelFactory *model_factory;
};



milkcat_model_t *milkcat_model_new(const char *model_path) {
  if (model_path == NULL) model_path = MODEL_PATH;

  milkcat_model_t *model = new milkcat_model_t;
  model->model_factory = new ModelFactory(model_path);

  return model;
}

milkcat_t *milkcat_new(milkcat_model_t *model, int analyzer_type) {
  global_status = Status::OK();

  milkcat_t *analyzer = new milkcat_t;
  memset(analyzer, 0, sizeof(milkcat_t));

  analyzer->model = model;

  switch (analyzer_type) {
   case DEFAULT_PROCESSOR:
    if (global_status.ok()) analyzer->segmenter = SegmenterFactory(
        analyzer->model->model_factory, 
        kMixedSegmenter, 
        global_status);
    if (global_status.ok()) analyzer->part_of_speech_tagger = PartOfSpeechTaggerFactory(
        analyzer->model->model_factory,
        kMixedPartOfSpeechTagger, 
        global_status);
    break;

   case CRF_SEGMENTER:
    if (global_status.ok()) analyzer->segmenter = SegmenterFactory(
        analyzer->model->model_factory, 
        kCrfSegmenter, 
        global_status);
    analyzer->part_of_speech_tagger = NULL;
    break;

   case CRF_PROCESSOR:
    if (global_status.ok()) analyzer->segmenter = SegmenterFactory(
        analyzer->model->model_factory, 
        kCrfSegmenter, 
        global_status);
    if (global_status.ok()) analyzer->part_of_speech_tagger = PartOfSpeechTaggerFactory(
        analyzer->model->model_factory, 
        kCrfPartOfSpeechTagger, 
        global_status);
    break;

   case DEFAULT_SEGMENTER:
    if (global_status.ok()) analyzer->segmenter = SegmenterFactory(
        analyzer->model->model_factory, 
        kMixedSegmenter, 
        global_status);
    analyzer->part_of_speech_tagger = NULL;
    break;

   case BIGRAM_SEGMENTER:
    if (global_status.ok()) analyzer->segmenter = SegmenterFactory(
        analyzer->model->model_factory, 
        kBigramSegmenter, 
        global_status);
    analyzer->part_of_speech_tagger = NULL;
    break;    

   default:
    global_status = Status::NotImplemented("");
    break;
  }

  if (!global_status.ok()) {
    milkcat_destroy(analyzer);
    return NULL;  
  } else {
    return analyzer;
  }
}

void milkcat_model_destory(milkcat_model_t *model) {
  delete model->model_factory;
  model->model_factory = NULL;

  delete model;
}

void milkcat_destroy(milkcat_t *m) {
  m->model = NULL;

  delete m->segmenter;
  m->segmenter = NULL;

  delete m->part_of_speech_tagger;
  m->part_of_speech_tagger = NULL;

  // Clear the cursor pool
  for (std::vector<Cursor *>::iterator it = m->cursor_pool.begin(); it != m->cursor_pool.end(); ++it) 
    delete *it;

  delete m;
}

void milkcat_model_set_userdict(milkcat_model_t *model, const char *path) {
  model->model_factory->SetUserDictionaryPath(path);
}


milkcat_cursor_t milkcat_analyze(milkcat_t *analyzer, const char *text) {
  milkcat_cursor_t cursor;
  Cursor *internal_cursor;

  if (analyzer->cursor_pool.empty()) {
    internal_cursor = new Cursor(analyzer);
  } else {
    internal_cursor = analyzer->cursor_pool.back();
    analyzer->cursor_pool.pop_back();
  }

  internal_cursor->Scan(text);
  cursor.internal_cursor = reinterpret_cast<void *>(internal_cursor);

  return cursor;
}

int milkcat_cursor_get_next(milkcat_cursor_t *cursor, milkcat_item_t *next_item) {
  Cursor *internal_cursor = reinterpret_cast<Cursor *>(cursor->internal_cursor);

  // If the cursor is already released
  if (internal_cursor == NULL) return MC_NONE;

  internal_cursor->MoveToNext();

  // If reached the end of text, collect back the cursor then return 
  // MC_NONE
  if (internal_cursor->end()) {
    cursor->internal_cursor = NULL;
    internal_cursor->analyzer()->cursor_pool.push_back(internal_cursor);
    return MC_NONE;
  } 

  next_item->word = internal_cursor->word();
  next_item->part_of_speech_tag = internal_cursor->part_of_speech_tag();
  next_item->word_type = internal_cursor->word_type();
  
  return MC_OK;
}

const char *milkcat_last_error() {
  return global_status.what();
}