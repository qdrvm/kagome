/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_SCALE_STREAM_HPP
#define KAGOME_CORE_SCALE_SCALE_STREAM_HPP

#include <deque>
#include <optional>

#include <boost/variant.hpp>
#include "common/byte_stream.hpp"
#include "scale/compact.hpp"

namespace kagome::scale {

  class ScaleStream : public common::ByteStream {
   public:
    template <class F, class S>
    ScaleStream &operator<<(const std::pair<F, S> &pair) {
      *this << pair.first << pair.second;
      return *this;
    }

    template <class... T>
    ScaleStream &operator<<(const boost::variant<T...> v) {
      //          *this <<
    }

    template <class T>
    ScaleStream &operator<<(const std::vector<T> &collection) {
      *this << compact::encodeInteger(collection.size());
      for (auto &&item : collection) {
        *this << item;
      }
      return *this;
    }

    template <class T> ScaleStream &operator<<(const std::optional<T> &v) {

    }

   private:
    std::deque<uint8_t> stream_;
  };

}  // namespace kagome::scale

#endif  // KAGOME_CORE_SCALE_SCALE_STREAM_HPP
