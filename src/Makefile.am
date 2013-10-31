AM_CPPFLAGS = -I$(top_srcdir)/include 
ACLOCAL_AMFLAGS = -I m4

include_HEADERS = milkcat.h
noinst_LIBRARIES = libmain.a
lib_LTLIBRARIES = libmilkcat.la
libmilkcat_la_LDFLAGS = -lstdc++ -lcrfpp
libmilkcat_la_SOURCES = milkcat_config.h \
                        crf_pos_tagger.h \
                        crf_segmenter.cc \
                        crf_segmenter.h \
                        crf_tagger.cc \
                        crf_tagger.h \
                        hmm_segment_and_pos_tagger.cc \
                        hmm_segment_and_pos_tagger.h \
                        instance_data.h \
                        instance_data.cc \
                        out_of_vocabulary_word_recognitioin.cc \
                        out_of_vocabulary_word_recognitioin.h \
                        pos_tag_set.cc \
                        pos_tag_set.h \
                        segment_tag_set.h \
                        tag_sequence.h \
                        tag_set.h \
                        term_instance.h \
                        term_instance.cc \
                        part_of_speech_tag_instance.h \
                        part_of_speech_tag_instance.cc \
                        token_instance.h \
                        token_instance.cc \
                        token_lex.cc \
                        token_lex.h \
                        tokenization.cc \
                        tokenization.h \
                        utils.cc \
                        utils.h \
                        libmilkcat.cc \
                        darts.h \
                        milkcat.h \
                        bigram_segmenter.h \
                        bigram_segmenter.cc \
                        feature_extractor.h \
                        crf_pos_tagger.cc\
                        strlcpy.cc\
                        strlcpy.h \
                        crfpp_model.h \
                        crfpp_model.cc \
                        crfpp_tagger.h \
                        crfpp_tagger.cc

bin_PROGRAMS = mkbigram
mkbigram_SOURCES = make_bigram_model.cc
libmain_a_SOURCES = main.c
