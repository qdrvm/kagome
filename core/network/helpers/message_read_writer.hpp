/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/assert.hpp>
#include <boost/system/error_code.hpp>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include "outcome/outcome.hpp"

namespace kagome::network {
  /**
   * NoSink is used to break the MessageReadWriter ancestor execute sequence.
   */
  struct NoSink {};

  /**
   * Chain specific messages read-writer
   */
  template <typename Adapter, typename Ancestor>
  struct MessageReadWriter final {
    using AncestorType = Ancestor;
    using AdapterType = Adapter;
    using BufferContainer = std::vector<uint8_t>;
    using SelfType = MessageReadWriter<AdapterType, AncestorType>;

   public:
    MessageReadWriter() = default;
    ~MessageReadWriter() = default;

    MessageReadWriter(MessageReadWriter &&) = default;
    MessageReadWriter &operator=(MessageReadWriter &&) = default;

    MessageReadWriter(const MessageReadWriter &) = delete;
    MessageReadWriter &operator=(const MessageReadWriter &) = delete;

   public:
    template <typename T>
    static size_t need_to_reserve(const T &t) {
      return AdapterType::size(t) + AncestorType::need_to_reserve(t);
    }

    template <typename T>
    static BufferContainer::iterator write(const T &t,
                                           BufferContainer &out,
                                           size_t reserved = 0ull) {
      const auto need_to_reserve = SelfType::need_to_reserve(t);
      if (need_to_reserve > out.size()) {
        out.resize(need_to_reserve);
      }

      const size_t r = AdapterType::size(t) + reserved;
      auto loaded = AncestorType::write(t, out, r);

      BOOST_ASSERT(std::distance(out.begin(), loaded) >= 0);
      BOOST_ASSERT(static_cast<size_t>(std::distance(out.begin(), loaded))
                   >= r);
      return AdapterType::write(t, out, loaded);
    }

    template <typename T>
    static outcome::result<BufferContainer::const_iterator> read(
        T &out,
        const BufferContainer &src,
        BufferContainer::const_iterator from) {
      if (from == src.end()) {
        return outcome::failure(std::errc::invalid_argument);
      }

      OUTCOME_TRY(it, AdapterType::read(out, src, from));
      return AncestorType::read(out, src, it);
    }
  };

  template <typename Adapter>
  struct MessageReadWriter<Adapter, NoSink> final {
    using AdapterType = Adapter;
    using BufferContainer = std::vector<uint8_t>;
    using SelfType = MessageReadWriter<AdapterType, NoSink>;

   public:
    MessageReadWriter() = default;
    ~MessageReadWriter() = default;

    MessageReadWriter(MessageReadWriter &&) = default;
    MessageReadWriter &operator=(MessageReadWriter &&) = default;

    MessageReadWriter(const MessageReadWriter &) = delete;
    MessageReadWriter &operator=(const MessageReadWriter &) = delete;

   public:
    template <typename T>
    static BufferContainer::iterator write(const T &t,
                                           BufferContainer &out,
                                           size_t reserved = 0) {
      const auto need_to_reserve = SelfType::need_to_reserve(t);
      if (need_to_reserve > out.size()) {
        out.resize(need_to_reserve);
      }

      BOOST_ASSERT(std::distance(out.begin(), out.end()) >= 0);
      BOOST_ASSERT(static_cast<size_t>(std::distance(out.begin(), out.end()))
                   >= AdapterType::size(t) + reserved);
      return AdapterType::write(t, out, out.end());
    }

    template <typename T>
    static outcome::result<BufferContainer::const_iterator> read(
        T &out,
        const BufferContainer &src,
        BufferContainer::const_iterator from) {
      if (from == src.end()) {
        return outcome::failure(std::errc::invalid_argument);
      }

      return AdapterType::read(out, src, from);
    }

    template <typename T>
    static size_t need_to_reserve(const T &t) {
      return AdapterType::size(t);
    }
  };

}  // namespace kagome::network
