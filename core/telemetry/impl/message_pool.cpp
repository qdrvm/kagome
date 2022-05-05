/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telemetry/impl/message_pool.hpp"

#include <mutex>  // for std::lock_guard

#include <boost/assert.hpp>

namespace kagome::telemetry {
  MessagePool::MessagePool(std::size_t entry_size_bytes,
                           std::size_t entries_count)
      : entry_size_{entry_size_bytes}, entries_count_{entries_count} {
    BOOST_ASSERT(entry_size_bytes > 0);
    BOOST_ASSERT(entries_count > 0);
    // preallocate all the buffers
    pool_.resize(entries_count);
    for (auto &entry : pool_) {
      entry.data.resize(entry_size_bytes, '\0');
    }
    free_slots_.reserve(entries_count);
    for (size_t i = 0; i < entries_count; ++i) {
      free_slots_.emplace(i);
    }
  }

  std::optional<MessageHandle> MessagePool::push(const std::string &message,
                                                 int16_t ref_count) {
    bool message_exceeds_max_size = message.length() > entry_size_;
    BOOST_ASSERT(not message_exceeds_max_size);
    if (message_exceeds_max_size) {
      return std::nullopt;
    }
    BOOST_VERIFY(ref_count >= 0);
    if (ref_count == 0) {
      return std::nullopt;
    }
    auto has_free_slot = nextFreeSlot();  // quick blocking lookup
    if (not has_free_slot) {
      return std::nullopt;
    }

    auto slot = has_free_slot.value();
    auto &entry = pool_[slot];
    entry.ref_count = ref_count;
    entry.data_size = message.length();
    memcpy(entry.data.data(), message.c_str(), message.length());
    // presence of zero-terminating char is preserved by the way of memory
    // (re-)initialization in constructor and release method, and also a
    // boundary check here - at the first line of push method
    return slot;
  }

  MessagePool::RefCount MessagePool::add_ref(MessageHandle handle) {
    bool handle_is_valid =
        (handle < pool_.size()) and (free_slots_.count(handle) == 0);
    BOOST_VERIFY(handle_is_valid);
    std::lock_guard lock(mutex_);
    // allowed to call only over already occupied slots
    BOOST_ASSERT(free_slots_.count(handle) == 0);
    auto &entry = pool_[handle];
    return ++entry.ref_count;
  }

  MessagePool::RefCount MessagePool::release(MessageHandle handle) {
    bool handle_is_valid =
        (handle < pool_.size()) and (free_slots_.count(handle) == 0);
    BOOST_VERIFY(handle_is_valid);
    std::lock_guard lock(mutex_);
    auto &entry = pool_[handle];
    if (entry.ref_count > 0 and --entry.ref_count == 0) {
      memset(entry.data.data(), '\0', entry.data.size());
      entry.data_size = 0;
      free_slots_.emplace(handle);
    }
    return entry.ref_count;
  }

  boost::asio::mutable_buffer MessagePool::operator[](
      MessageHandle handle) const {
    bool handle_is_valid =
        (handle < pool_.size()) and (free_slots_.count(handle) == 0);
    BOOST_VERIFY(handle_is_valid);
    // No synchronization required due to the design of its use way.
    // Read access might be requested only when a handle is already acquired.
    // => There would be no race during initialization and read.
    // The buffer will remain valid till all holders request its release.
    // The handle cannot be reassigned prior to complete release.
    // => There is no chance to get dangling pointers inside boost buffers.
    auto &entry = pool_[handle];
    return boost::asio::buffer(const_cast<uint8_t *>(entry.data.data()),
                               entry.data_size);
  }

  std::size_t MessagePool::capacity() const {
    return entries_count_;
  }

  std::optional<MessageHandle> MessagePool::nextFreeSlot() {
    std::lock_guard lock(mutex_);
    if (free_slots_.empty()) {
      return std::nullopt;
    }
    auto free_slot = free_slots_.begin();
    auto slot = *free_slot;
    free_slots_.erase(free_slot);
    return slot;
  }
}  // namespace kagome::telemetry
