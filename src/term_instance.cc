/*
 * term_instance.cc
 *
 * by ling0322 at 2013-10-20
 *
 */

#include "term_instance.h"
#include "milkcat_config.h"

TermInstance::TermInstance() {
  instance_data_ = new InstanceData(1, 3, kTokenMax);
} 

TermInstance::~TermInstance() {
  delete instance_data_;
}