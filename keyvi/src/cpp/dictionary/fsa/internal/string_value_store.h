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
 * string_value_store.h
 *
 *  Created on: Jul 16, 2014
 *      Author: hendrik
 */

#ifndef STRING_VALUE_STORE_H_
#define STRING_VALUE_STORE_H_

#include <boost/functional/hash.hpp>

#include "dictionary/fsa/internal/ivalue_store.h"
#include "dictionary/fsa/internal/serialization_utils.h"
#include "dictionary/fsa/internal/minimization_hash.h"

//#define ENABLE_TRACING
#include "dictionary/util/trace.h"

namespace keyvi {
namespace dictionary {
namespace fsa {
namespace internal {

/**
 * Value store where the value consists of a string.
 */
class StringValueStore
final {
   public:

    struct StringPointer
    final {
       public:
        StringPointer()
            : StringPointer(0, 0, 0) {
        }

        StringPointer(uint64_t offset, int hashcode, ushort length)
            : offset_(offset),
              hashcode_(hashcode),
              length_(length) {
        }

        int GetHashcode() const {
          return hashcode_;
        }

        uint64_t GetOffset() const {
          return offset_;
        }

        ushort GetLength() const {
          return length_;
        }

        int GetCookie() const {
          return cookie_;
        }

        void SetCookie(int value) {
          cookie_ = static_cast<ushort>(value);
        }

        bool IsEmpty() const {
          return offset_ == 0 && hashcode_ == 0 && length_ == 0;
        }

        bool operator==(const StringPointer& l) {
          return offset_ == l.offset_;
        }

        static size_t GetMaxCookieSize(){
          return MaxCookieSize;
        }

       private:
        static const size_t MaxCookieSize = 0xFFFF;

        uint64_t offset_;
        int32_t hashcode_;
        ushort length_;
        ushort cookie_ = 0;
      };

      template<class PersistenceT>
      struct StringPointerForCompare
      final
      {
         public:
          StringPointerForCompare(const std::string& value,
                                  PersistenceT* persistence)
              : value_(value),
                persistence_(persistence)
         {
            hashcode_ = std::hash<value_t>()(value);
            length_ = value.size();
          }

          int GetHashcode() const {
            return hashcode_;
          }

          bool operator==(const StringPointer& l) const {
            // First filter - check if hash code  is the same
            if (l.GetHashcode() != hashcode_) {
              return false;
            }

            size_t length_l = l.GetLength();

            if (length_l < USHRT_MAX && length_l != length_) {
              return false;
            }

            size_t offset = l.GetOffset();

            // ensure not to go over memory boundaries
            if (persistence_->size() < offset + length_ ) {
              return false;
            }

            for (size_t i = 0; i < length_; ++i) {
              char c = persistence_->operator[](offset+i);
              if (c != value_[i]) {
                return false;
              }
            }

            // strings must be equal
            return true;
          }

         private:
          std::string value_;
          PersistenceT* persistence_;
          int32_t hashcode_;
          size_t length_;
        };

        typedef std::string value_t;
        static const uint64_t no_value = 0;
        static const bool inner_weight = false;

        StringValueStore(boost::filesystem::path temporary_path = "")
        {}

        StringValueStore& operator=(StringValueStore const&) = delete;
        StringValueStore(const StringValueStore& that) = delete;

        /**
         * Simple implementation of a value store for strings:
         * todo: performance improvements / port stuff from json_value_store
         */
        uint64_t GetValue(const value_t& value, bool& no_minimization) {
          const StringPointerForCompare<std::vector<char>> stp(value, &string_values_);

          const StringPointer p = hash_.Get(stp);

          if (!p.IsEmpty()){
            // found the same value again, minimize
            return p.GetOffset();
          }

          no_minimization = true;

          // else persist string value
          uint64_t pt = static_cast<uint64_t>(string_values_.size());

          for (size_t i = 0; i < value.size(); ++i) {
            string_values_.push_back(value.c_str()[i]);
          }

          // add zero termination
          string_values_.push_back(0);

          hash_.Add(StringPointer(pt,stp.GetHashcode(),value.size()));

          return pt;
        }

        uint32_t GetWeightValue(value_t value) const {
          return 0;
        }

        value_store_t GetValueStoreType() const {
          return STRING_VALUE_STORE;
        }

        void Write(std::ostream& stream) {

          boost::property_tree::ptree pt;
          pt.put("size", std::to_string(string_values_.size()));

          internal::SerializationUtils::WriteJsonRecord(stream, pt);
          TRACE("Wrote JSON header, stream at %d", stream.tellp());

          stream.write((const char*) &string_values_[0], string_values_.size());
        }

       private:
        std::vector<char> string_values_;
        MinimizationHash<StringPointer> hash_;
      };

      class StringValueStoreReader final: public IValueStoreReader {
       public:
        using IValueStoreReader::IValueStoreReader;

        StringValueStoreReader(std::istream& stream,
                               boost::interprocess::file_mapping* file_mapping)
            : IValueStoreReader(stream, file_mapping) {

          boost::property_tree::ptree properties =
              internal::SerializationUtils::ReadJsonRecord(stream);

          size_t offset = stream.tellg();
          size_t strings_size = properties.get<size_t>("size");

          strings_region_ = new boost::interprocess::mapped_region(
              *file_mapping, boost::interprocess::read_only, offset,
              strings_size);

          strings_ = (const char*) strings_region_->get_address();
        }

        ~StringValueStoreReader() {
          delete strings_region_;
        }

        virtual attributes_t GetValueAsAttributeVector(uint64_t fsa_value)
            override {
          attributes_t attributes(new attributes_raw_t());

          std::string raw_value(strings_ + fsa_value);

          (*attributes)["value"] = raw_value;
          return attributes;
        }

        virtual std::string GetValueAsString(uint64_t fsa_value) override {
          return std::string(strings_ + fsa_value);
        }

       private:
        boost::interprocess::mapped_region* strings_region_;
        const char* strings_;
      };

} /* namespace internal */
} /* namespace fsa */
} /* namespace dictionary */
} /* namespace keyvi */

#endif /* STRING_VALUE_STORE_H_ */
