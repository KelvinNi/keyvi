/* * keyvi - A key value store.
 *
 * Copyright 2015 Hendrik Muhs<hendrik.muhs@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * null_value_store.h
 *
 *  Created on: May 13, 2014
 *      Author: hendrik
 */

#ifndef NULL_VALUE_STORE_H_
#define NULL_VALUE_STORE_H_

#include "dictionary/fsa/internal/ivalue_store.h"

namespace keyvi {
namespace dictionary {
namespace fsa {
namespace internal {

/**
 * A value store for key-only dictionaries.
 * Simply return 0 for all values.
 */
class NullValueStore final{
 public:
  typedef uint32_t value_t;
  static const uint64_t no_value = 0;
  static const bool inner_weight = false;

  NullValueStore(boost::filesystem::path temporary_path = ""){
  }

  NullValueStore& operator=(NullValueStore const&) = delete;
  NullValueStore(const NullValueStore& that) = delete;


  uint64_t GetValue(value_t value, bool& no_minimization){
    return 0;
  }

  uint32_t GetWeightValue(value_t value){
    return 0;
  }

  value_store_t GetValueStoreType(){
    return NULL_VALUE_STORE;
  }

  void Write(std::ostream& stream) {}
};

class NullValueStoreReader final: public IValueStoreReader{
 public:
  using IValueStoreReader::IValueStoreReader;

  virtual attributes_t GetValueAsAttributeVector(uint64_t fsa_value) override {
    return attributes_t();
  }

  virtual std::string GetValueAsString(uint64_t fsa_value) override {
    return "";
  }
};

} /* namespace internal */
} /* namespace fsa */
} /* namespace dictionary */
} /* namespace keyvi */


#endif /* NULL_VALUE_STORE_H_ */
