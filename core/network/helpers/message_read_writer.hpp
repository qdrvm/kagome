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
  /**
   * Chain specific messages read-writer
   */
  template<typename Adapter, typename Ancestor> struct MessageReadWriter final {
    using AncestorType = Ancestor;
    using AdapterType = Adapter;
    using BufferContainer = std::vector<uint8_t>;

   public:
    MessageReadWriter() = default;

    MessageReadWriter(MessageReadWriter &&) = default;
    MessageReadWriter& operator=(MessageReadWriter &&) = default;

    MessageReadWriter(const MessageReadWriter&) = delete;
    MessageReadWriter& operator=(const MessageReadWriter&) = delete;

   public:
    template<typename T>
    BufferContainer::iterator write(const T &t, BufferContainer &out, size_t reserved = 0) {
      constexpr size_t r = AdapterType<T>::size(t) + reserved;
      out.resize(r);

      BufferContainer::iterator loaded = sink_->write(t, out, r);
      assert(std::distance(out.begin(), loaded) >= r);
      return AdapterType<T>::write(t, out, loaded);
    }

   private:
    AncestorType sink_;
  };

  template<typename Adapter> struct MessageReadWriter final {
    using AncestorType = Ancestor;
    using AdapterType = Adapter;
    using BufferContainer = std::vector<uint8_t>;

   public:
    MessageReadWriter() = default;

    MessageReadWriter(MessageReadWriter &&) = default;
    MessageReadWriter& operator=(MessageReadWriter &&) = default;

    MessageReadWriter(const MessageReadWriter&) = delete;
    MessageReadWriter& operator=(const MessageReadWriter&) = delete;

   public:
    template<typename T>
    BufferContainer::iterator write(const T &t, BufferContainer &out, size_t reserved = 0) {
      constexpr size_t r = AdapterType<T>::size(t) + reserved;
      out.resize(r);
      return AdapterType<T>::write(t, out, out.end());
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_SCALE_MESSAGE_READ_WRITER_HPP
