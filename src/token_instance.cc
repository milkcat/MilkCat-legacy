/*
 * token_instance.cc
 *
 * by ling0322 at 2013-10-20
 *
 */

#include "token_instance.h"
#include "milkcat_config.h"

TokenInstance::TokenInstance() {
  instance_data_ = new InstanceData(1, 1, kTokenMax);
}

TokenInstance::~TokenInstance() {
  delete instance_data_;
}