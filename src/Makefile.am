AM_CPPFLAGS = -I$(top_srcdir)/include -DMODEL_PATH=\"$(pkgdatadir)/\"
ACLOCAL_AMFLAGS = -I m4

include_HEADERS = milkcat.h
noinst_LIBRARIES = libmain.a
lib_LTLIBRARIES = libmilkcat.la
libmilkcat_la_LDFLAGS = -lstdc++
libmilkcat_la_SOURCES = milkcat_config.h \
                        crf_segmenter.cc \
                        crf_segmenter.h \
                        instance_data.h \
                        instance_data.cc \
                        out_of_vocabulary_word_recognition.cc \
                        out_of_vocabulary_word_recognition.h \
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
                        crf_part_of_speech_tagger.h \
                        crf_part_of_speech_tagger.cc\
                        strlcpy.cc\
                        crf_model.h \
                        crf_model.cc \
                        crf_tagger.h \
                        crf_tagger.cc \
                        configuration.h \
                        configuration.cc \
                        hmm_part_of_speech_tagger.h \
                        hmm_part_of_speech_tagger.cc \
                        mixed_part_of_speech_tagger.h \
                        mixed_part_of_speech_tagger.cc \
                        static_hash_table.h \
                        segmenter.h \
                        part_of_speech_tagger.h \
                        mixed_segmenter.h \
                        mixed_segmenter.cc \
                        hmm_model.cc \
                        hmm_model.h

bin_PROGRAMS = mc_model
mc_model_SOURCES = mk_model.cc 
mc_model_LDADD = libmilkcat.la
libmain_a_SOURCES = main.c
