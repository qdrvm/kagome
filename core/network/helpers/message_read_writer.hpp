/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_READ_WRITER_HPP
#define KAGOME_MESSAGE_READ_WRITER_HPP

#include <functional>
#include <memory>
#include <gsl/span>

namespace kagome::network {
  struct NoSink {};

  /**
   * Chain specific messages read-writer
   */
  template<typename Adapter, typename Ancestor> struct MessageReadWriter final {
    using AncestorType = Ancestor;
    using AdapterType = Adapter;
    using BufferContainer = std::vector<uint8_t>;
    using SelfType = MessageReadWriter<AdapterType, AncestorType>;

   public:
    MessageReadWriter() = default;

    MessageReadWriter(MessageReadWriter &&) = default;
    MessageReadWriter& operator=(MessageReadWriter &&) = default;

    MessageReadWriter(const MessageReadWriter&) = delete;
    MessageReadWriter& operator=(const MessageReadWriter&) = delete;

   public:
    template<typename T>
    static size_t need_to_reserve(const T &t) {
      return AdapterType::size(t) + AncestorType::need_to_reserve(t);
    }

    template<typename T>
    static BufferContainer::iterator write(const T &t, BufferContainer &out, size_t reserved = 0ull) {
      const auto need_to_reserve = SelfType::need_to_reserve(t);
      if (need_to_reserve > out.size())
        out.resize(need_to_reserve);

      const size_t r = AdapterType::size(t) + reserved;
      BufferContainer::iterator loaded = AncestorType::write(t, out, r);

      assert(std::distance(out.begin(), loaded) >= r);
      return AdapterType::write(t, out, loaded);
    }

    template<typename T>
    static s
  };

  template<typename Adapter> struct MessageReadWriter<Adapter, NoSink> final {
    using AdapterType = Adapter;
    using BufferContainer = std::vector<uint8_t>;
    using SelfType = MessageReadWriter<AdapterType, NoSink>;

   public:
    MessageReadWriter() = default;

    MessageReadWriter(MessageReadWriter &&) = default;
    MessageReadWriter& operator=(MessageReadWriter &&) = default;

    MessageReadWriter(const MessageReadWriter&) = delete;
    MessageReadWriter& operator=(const MessageReadWriter&) = delete;

   public:
    template<typename T>
    static BufferContainer::iterator write(const T &t, BufferContainer &out, size_t reserved = 0) {
      const auto need_to_reserve = SelfType::need_to_reserve(t);
      if (need_to_reserve > out.size())
        out.resize(need_to_reserve);

      const size_t r = AdapterType::size(t) + reserved;
      assert(std::distance(out.begin(), out.end()) >= r);
      return AdapterType::write(t, out, out.end());
    }

    template<typename T>
    static size_t need_to_reserve(const T &t) {
      return AdapterType::size(t);
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_SCALE_MESSAGE_READ_WRITER_HPP
