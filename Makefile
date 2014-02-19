PREFIX ?= /usr/local
MODEL_DIR ?= $(PREFIX)/share/milkcat/
LIBRARY_DIR ?= $(PREFIX)/lib
BINARY_DIR ?= $(PREFIX)/bin
INCLUDE_DIR ?= $(PREFIX)/include

BUILD_DIR ?= build

CFLAGS += -g -fPIC -O3 -pthread -Isrc -MP -MD
CXXFLAGS += $(CFLAGS) -std=c++11 -fno-rtti -fno-exceptions

.PHONY: all clean
all: $(BUILD_DIR)/libmilkcat.so $(BUILD_DIR)/milkcat $(BUILD_DIR)/neko

LIBMILKCAT_SOURCES = $(wildcard src/milkcat/*.cc src/utils/*.cc src/neko/*.cc)
LIBMILKCAT_OBJECTS = $(addprefix $(BUILD_DIR)/, $(LIBMILKCAT_SOURCES:.cc=.o))
LIBMILKCAT_LDFLAGS = -shared -pthread
LIBMILKCAT_CXXFLAGS = $(CXXFLAGS) -DMODEL_PATH=\"$(MODEL_DIR)\"
-include $(LIBMILKCAT_OBJECTS:.o=.d)

$(LIBMILKCAT_OBJECTS): $(BUILD_DIR)/%.o: %.cc
	@if [ ! -d $(dir $@) ];then mkdir -p $(dir $@); fi
	$(CXX) $(LIBMILKCAT_CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/libmilkcat.so: $(LIBMILKCAT_OBJECTS)
	$(CXX) $(LIBMILKCAT_LDFLAGS) -o $@ $(LIBMILKCAT_OBJECTS)



MILKCAT_SOURCES = src/main.c
MILKCAT_CFLAGS = $(CFLAGS)
MILKCAT_LDFLAGS = -pthread -L$(BUILD_DIR) -lmilkcat
-include $(BUILD_DIR)/milkcat.d

$(BUILD_DIR)/milkcat: $(MILKCAT_SOURCES)
	$(CC) $(MILKCAT_CFLAGS) -o $@ $< $(MILKCAT_LDFLAGS)


NEKO_SOURCES = src/neko_main.cc
NEKO_CXXFLAGS = $(CXXFLAGS)
NEKO_LDFLAGS = -pthread -L$(BUILD_DIR) -lmilkcat -DMODEL_PATH=\"$(MODEL_DIR)\"
-include $(BUILD_DIR)/neko.d

$(BUILD_DIR)/neko: $(NEKO_SOURCES)
	$(CXX) $(NEKO_CXXFLAGS) -o $@ $< $(NEKO_LDFLAGS)


clean:
	rm -rf build

install: all
	if [ ! -d $(MODEL_DIR) ];then mkdir -p $(MODEL_DIR); fi
	cp data/* $(MODEL_DIR)
	cp $(BUILD_DIR)/milkcat $(BINARY_DIR)
	cp $(BUILD_DIR)/neko $(BINARY_DIR)
	# cp build/out/Default/mk_model $(BINARY_DIR)
	cp $(BUILD_DIR)/libmilkcat.so $(LIBRARY_DIR)
	cp src/milkcat/milkcat.h $(INCLUDE_DIR)

uninstall:
	-rm $(MODEL_DIR)/*
	-rm $(BINARY_DIR)/milkcat
	# rm $(BINARY_DIR)/mk_model
	-rm $(BINARY_DIR)/neko
	-rm $(LIBRARY_DIR)/libmilkcat.*
	-rm $(INCLUDE_DIR)/milkcat.h


CPPLINT_EXCLUDE ?=
CPPLINT_EXCLUDE += src/milkcat/token_lex.cc
CPPLINT_EXCLUDE += src/milkcat/token_lex.h
CPPLINT_EXCLUDE += src/milkcat/darts.h
CPPLINT_EXCLUDE += src/neko/utf8.h

ALL_CPP_FILES = $(wildcard src/*.cc \
                           src/*.h \
                           src/milkcat/*.cc \
                           src/milkcat/*.h \
                           src/utils/*.cc \
                           src/utils/*.h \
                           src/neko/*.cc \
                           src/neko/*.h)
CPPLINT_FILES = $(filter-out $(CPPLINT_EXCLUDE), $(ALL_CPP_FILES))

lint:
	$(PYTHON) tools/cpplint.py $(CPPLINT_FILES)
