PREFIX ?= /usr/local
MODEL_DIR ?= $(PREFIX)/share/milkcat
LIBRARY_DIR ?= $(PREFIX)/lib
BINARY_DIR ?= $(PREFIX)/bin
PYTHON ?= python
V ?= 0

all: milkcat

gyp_build: milkcat.gyp
	if [ ! -d build ];then mkdir build; fi
	tools/gyp/gyp --depth=. milkcat.gyp -f make --generator-output=build -D model_path=$(MODEL_DIR)/
	make -C build

milkcat: gyp_build

clean:
	rm -rf build

install:
	if [ ! -d $(MODEL_DIR) ];then mkdir -p $(MODEL_DIR); fi
	cp data/* $(MODEL_DIR)
	cp build/out/Default/milkcat $(BINARY_DIR)
	cp build/out/Default/neko $(BINARY_DIR)
	cp build/out/Default/mk_model $(BINARY_DIR)
	cp build/out/Default/obj.target/libmilkcat.* $(LIBRARY_DIR)

uninstall:
	rm $(MODEL_DIR)/*
	rm $(BINARY_DIR)/milkcat
	rm $(BINARY_DIR)/mk_model
	rm $(BINARY_DIR)/neko
	rm $(LIBRARY_DIR)/libmilkcat.*


CPPLINT_EXCLUDE ?=
CPPLINT_EXCLUDE += src/node_dtrace.cc
CPPLINT_EXCLUDE += src/node_dtrace.cc
CPPLINT_EXCLUDE += src/node_root_certs.h
CPPLINT_EXCLUDE += src/node_win32_perfctr_provider.cc
CPPLINT_EXCLUDE += src/queue.h
CPPLINT_EXCLUDE += src/tree.h
CPPLINT_EXCLUDE += src/v8abbr.h

CPPLINT_FILES = $(filter-out $(CPPLINT_EXCLUDE), $(wildcard src/*.cc src/*.h src/*.c))

lint:
	$(PYTHON) tools/cpplint.py $(CPPLINT_FILES)
