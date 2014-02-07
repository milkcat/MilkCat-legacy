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
#include "libmilkcat.h"

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
  const StaticArray<float> *user_unigram_cost = nullptr;
  const StaticHashTable<int64_t, float> *bigram_cost = NULL;
  const CRFModel *seg_model = NULL;
  const TrieTree *oov_property = NULL;

  switch (segmenter_id) {
   case kBigramSegmenter:
    if (status.ok()) index = factory->Index(status);
    if (status.ok() && factory->HasUserDictionary()) user_index = factory->UserIndex(status);
    if (status.ok() && factory->HasUserDictionary()) user_unigram_cost = factory->UserCost(status);
    if (status.ok()) unigram_cost = factory->UnigramCost(status);
    if (status.ok()) bigram_cost = factory->BigramCost(status);

    if (status.ok()) {
      return new BigramSegmenter(index, user_index, unigram_cost, user_unigram_cost, bigram_cost);
    } else {
      return NULL;
    }

   case kCrfSegmenter:
    if (status.ok()) seg_model = factory->CRFSegModel(status);

    if (status.ok()) {
      return CRFSegmenter::New(seg_model, status);
    } else {
      return NULL;
    }

   case kMixedSegmenter:
    if (status.ok()) index = factory->Index(status);
    if (status.ok() && factory->HasUserDictionary()) user_index = factory->UserIndex(status);
    if (status.ok() && factory->HasUserDictionary()) user_unigram_cost = factory->UserCost(status);
    if (status.ok()) unigram_cost = factory->UnigramCost(status);
    if (status.ok()) bigram_cost = factory->BigramCost(status);
    if (status.ok()) seg_model = factory->CRFSegModel(status);
    if (status.ok()) oov_property = factory->OOVProperty(status);

    if (status.ok()) {
      return MixedSegmenter::New(
        index,
        user_index,
        unigram_cost,
        user_unigram_cost,
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
    if (status.ok()) crf_pos_model = factory->CRFPosModel(status);

    if (status.ok()) {
      return new CRFPartOfSpeechTagger(crf_pos_model);
    } else {
      return NULL;
    }

   case kHmmPartOfSpeechTagger:
    if (status.ok()) hmm_pos_model = factory->HMMPosModel(status);
    if (status.ok()) index = factory->Index(status);
    if (status.ok()) default_tag = factory->DefaultTag(status);

    if (status.ok()) {
      return HMMPartOfSpeechTagger::New(hmm_pos_model, index, default_tag, status);  
    } else {
      return NULL;
    }

   case kMixedPartOfSpeechTagger:
    if (status.ok()) crf_pos_model = factory->CRFPosModel(status);
    if (status.ok()) hmm_pos_model = factory->HMMPosModel(status);
    if (status.ok()) index = factory->Index(status);
    if (status.ok()) default_tag = factory->DefaultTag(status);

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

// ---------- ModelFactory ----------

ModelFactory::ModelFactory(const char *model_dir_path): 
    model_dir_path_(model_dir_path),
    unigram_index_(nullptr),
    unigram_cost_(nullptr),
    user_cost_(nullptr),
    bigram_cost_(nullptr),
    seg_model_(nullptr),
    crf_pos_model_(nullptr),
    hmm_pos_model_(nullptr),
    oov_property_(nullptr),
    user_index_(nullptr),
    default_tag_(nullptr) {

  pthread_mutex_init(&mutex, nullptr);
}

ModelFactory::~ModelFactory() {
  pthread_mutex_destroy(&mutex);

  delete user_index_;
  user_index_ = nullptr;

  delete unigram_index_;
  unigram_index_ = nullptr;

  delete unigram_cost_;
  unigram_cost_ = nullptr;

  delete user_cost_;
  user_cost_ = nullptr;

  delete bigram_cost_;
  bigram_cost_ = nullptr;

  delete seg_model_;
  seg_model_ = nullptr;

  delete crf_pos_model_;
  crf_pos_model_ = nullptr;

  delete hmm_pos_model_;
  hmm_pos_model_ = nullptr;

  delete oov_property_;
  oov_property_ = nullptr;

  delete default_tag_;
  default_tag_ = nullptr;    
}

const TrieTree *ModelFactory::Index(Status &status) {
  pthread_mutex_lock(&mutex);
  if (unigram_index_ == nullptr) {
    unigram_index_ = DoubleArrayTrieTree::New((model_dir_path_ + UNIGRAM_INDEX).c_str(), status);
  }
  pthread_mutex_unlock(&mutex);
  return unigram_index_;
}

void ModelFactory::LoadUserDictionary(Status &status) {
  char line[1024], word[1024];
  std::string errmsg;
  ReadableFile *fd;
  float default_cost = kDefaultCost, cost;
  std::vector<float> user_costs;
  std::map<std::string, int> term_ids;

  if (user_dictionary_path_ == "") {
    status = Status::RuntimeError("No user dictionary.");
    return ;
  }

  if (status.ok()) fd = ReadableFile::New(user_dictionary_path_.c_str(), status);
  while (status.ok() && !fd->Eof()) {
    fd->ReadLine(line, sizeof(line), status);
    if (status.ok()) {
      char *p = strchr(line, ' ');

      // Checks if the entry has a cost 
      if (p != nullptr) {
        strlcpy(word, line, p - line + 1);
        trim(word);
        trim(p);
        cost = static_cast<float>(atof(p));
      } else {
        strlcpy(word, line, sizeof(word));
        trim(word);   
        cost = default_cost;
      }
      term_ids.insert(std::pair<std::string, int>(word, kUserTermIdStart + term_ids.size()));
      user_costs.push_back(cost);
    }
  }

  if (status.ok() && term_ids.size() == 0) {
    errmsg = std::string("user dictionary ") + user_dictionary_path_ + " is empty.";
    status = Status::Corruption(errmsg.c_str());
  }

  
  // Build the index and the cost array from user dictionary
  if (status.ok()) user_index_ = DoubleArrayTrieTree::NewFromMap(term_ids);
  if (status.ok()) user_cost_ = StaticArray<float>::NewFromArray(user_costs.data(), user_costs.size());

  delete fd;
}

const TrieTree *ModelFactory::UserIndex(Status &status) {
  pthread_mutex_lock(&mutex);
  if (user_index_ == nullptr) {
    LoadUserDictionary(status);
  }
  pthread_mutex_unlock(&mutex);
  return user_index_;
} 

const StaticArray<float> *ModelFactory::UserCost(Status &status) {
  pthread_mutex_lock(&mutex);
  if (user_cost_ == nullptr) {
    LoadUserDictionary(status);
  }
  pthread_mutex_unlock(&mutex);
  return user_cost_;
} 

const StaticArray<float> *ModelFactory::UnigramCost(Status &status) {
  pthread_mutex_lock(&mutex);
  if (unigram_cost_ == NULL) {
    unigram_cost_ = StaticArray<float>::New((model_dir_path_ + UNIGRAM_DATA).c_str(), status);
  }
  pthread_mutex_unlock(&mutex);
  return unigram_cost_;
}

const StaticHashTable<int64_t, float> *ModelFactory::BigramCost(Status &status) {
  pthread_mutex_lock(&mutex);
  if (bigram_cost_ == NULL) {
    bigram_cost_ = StaticHashTable<int64_t, float>::New((model_dir_path_ + BIGRAM_DATA).c_str(), status);
  }
  pthread_mutex_unlock(&mutex);
  return bigram_cost_;
}

const CRFModel *ModelFactory::CRFSegModel(Status &status) {
  pthread_mutex_lock(&mutex);
  if (seg_model_ == NULL) {
    seg_model_ = CRFModel::New((model_dir_path_ + CRF_SEGMENTER_MODEL).c_str(), status);
  }
  pthread_mutex_unlock(&mutex);
  return seg_model_;   
}

const CRFModel *ModelFactory::CRFPosModel(Status &status) {
  pthread_mutex_lock(&mutex);
  if (crf_pos_model_ == NULL) {
    crf_pos_model_ = CRFModel::New((model_dir_path_ + CRF_PART_OF_SPEECH_MODEL).c_str(), status);
  }
  pthread_mutex_unlock(&mutex);
  return crf_pos_model_;   
}

const HMMModel *ModelFactory::HMMPosModel(Status &status) {
  pthread_mutex_lock(&mutex);
  if (hmm_pos_model_ == NULL) {
    hmm_pos_model_ = HMMModel::New((model_dir_path_ + HMM_PART_OF_SPEECH_MODEL).c_str(), status);
  }
  pthread_mutex_unlock(&mutex);
  return hmm_pos_model_;   
}

const TrieTree *ModelFactory::OOVProperty(Status &status) {
  pthread_mutex_lock(&mutex);
  if (oov_property_ == NULL) {
    oov_property_ = DoubleArrayTrieTree::New((model_dir_path_ + OOV_PROPERTY).c_str(), status);
  }
  pthread_mutex_unlock(&mutex);
  return oov_property_;
}

const Configuration *ModelFactory::DefaultTag(Status &status) {
  pthread_mutex_lock(&mutex);
  if (default_tag_ == NULL) {
    default_tag_ = Configuration::New((model_dir_path_ + DEFAULT_TAG).c_str(), status);
  }
  pthread_mutex_unlock(&mutex);
  return default_tag_;
}

// ---------- Cursor ----------

Cursor::Cursor(milkcat_t *analyzer):
    analyzer_(analyzer),
    tokenizer_(TokenizerFactory(kDefaultTokenizer)),
    token_instance_(new TokenInstance()),
    term_instance_(new TermInstance()),
    part_of_speech_tag_instance_(new PartOfSpeechTagInstance()),
    sentence_length_(0),
    current_position_(0),
    end_(0) {  
}

Cursor::~Cursor() {
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

void Cursor::Scan(const char *text) {
  tokenizer_->Scan(text);
  sentence_length_ = 0;
  current_position_ = 0;
  end_ = false;
}

void Cursor::MoveToNext() {
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

// ---------- Fucntions in milkcat.h ----------

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
    analyzer->part_of_speech_tagger = nullptr;
    break;    

   default:
    global_status = Status::NotImplemented("");
    break;
  }

  if (!global_status.ok()) {
    milkcat_destroy(analyzer);
    return nullptr;  
  } else {
    return analyzer;
  }
}

void milkcat_model_destroy(milkcat_model_t *model) {
  if (model == nullptr) return ;

  delete model->model_factory;
  model->model_factory = nullptr;

  delete model;
}

void milkcat_destroy(milkcat_t *analyzer) {
  if (analyzer == nullptr) return ;

  analyzer->model = nullptr;

  delete analyzer->segmenter;
  analyzer->segmenter = nullptr;

  delete analyzer->part_of_speech_tagger;
  analyzer->part_of_speech_tagger = nullptr;

  // Clear the cursor pool
  for (auto &cursor: analyzer->cursor_pool) {
    delete cursor;
  } 

  delete analyzer;
}

void milkcat_model_set_userdict(milkcat_model_t *model, const char *path) {
  model->model_factory->SetUserDictionary(path);
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