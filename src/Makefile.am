AUTOMAKE_OPTIONS = gnu no-dependencies subdir-objects
AM_CXXFLAGS = -DMODEL_PATH=\"$(pkgdatadir)/\" -std=c++11 -fno-rtti \
              -fno-exceptions
ACLOCAL_AMFLAGS = -I m4

include_HEADERS = milkcat/milkcat.h

lib_LTLIBRARIES = libmilkcat.la
libmilkcat_la_SOURCES = milkcat/milkcat_config.h \
                        milkcat/crf_segmenter.cc \
                        milkcat/crf_segmenter.h \
                        milkcat/instance_data.h \
                        milkcat/instance_data.cc \
                        milkcat/out_of_vocabulary_word_recognition.cc \
                        milkcat/out_of_vocabulary_word_recognition.h \
                        milkcat/term_instance.h \
                        milkcat/term_instance.cc \
                        milkcat/part_of_speech_tag_instance.h \
                        milkcat/part_of_speech_tag_instance.cc \
                        milkcat/token_instance.h \
                        milkcat/token_instance.cc \
                        milkcat/token_lex.cc \
                        milkcat/token_lex.h \
                        milkcat/tokenizer.cc \
                        milkcat/tokenizer.h \
                        milkcat/milkcat.h \
                        milkcat/libmilkcat.cc \
                        milkcat/darts.h \
                        milkcat/milkcat.h \
                        milkcat/bigram_segmenter.h \
                        milkcat/bigram_segmenter.cc \
                        milkcat/feature_extractor.h \
                        milkcat/crf_part_of_speech_tagger.h \
                        milkcat/crf_part_of_speech_tagger.cc \
                        milkcat/crf_model.h \
                        milkcat/crf_model.cc \
                        milkcat/crf_tagger.h \
                        milkcat/crf_tagger.cc \
                        milkcat/configuration.h \
                        milkcat/configuration.cc \
                        milkcat/hmm_part_of_speech_tagger.h \
                        milkcat/hmm_part_of_speech_tagger.cc \
                        milkcat/mixed_part_of_speech_tagger.h \
                        milkcat/mixed_part_of_speech_tagger.cc \
                        milkcat/static_hash_table.h \
                        milkcat/segmenter.h \
                        milkcat/part_of_speech_tagger.h \
                        milkcat/mixed_segmenter.h \
                        milkcat/mixed_segmenter.cc \
                        milkcat/hmm_model.cc \
                        milkcat/hmm_model.h \
                        milkcat/trie_tree.h \
                        milkcat/trie_tree.cc \
                        milkcat/static_array.h \
                        utils/status.h \
                        utils/utils.cc \
                        utils/utils.h \
                        utils/strlcpy.cc \
                        utils/readable_file.cc \
                        utils/readable_file.h \
                        utils/writable_file.cc \
                        utils/writable_file.h \
                        neko/crf_vocab.h \
                        neko/crf_vocab.cc \
                        neko/candidate.cc \
                        neko/candidate.h \
                        neko/maxent_classifier.cc \
                        neko/maxent_classifier.h \
                        neko/mutual_information.cc \
                        neko/mutual_information.h \
                        neko/final_rank.cc \
                        neko/final_rank.h \
                        neko/bigram_anal.cc \
                        neko/bigram_anal.h \
                        neko/utf8.h
