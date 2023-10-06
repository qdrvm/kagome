/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MESSAGE_POOL_HPP
#define KAGOME_MESSAGE_POOL_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>

#include <boost/asio/buffer.hpp>
#include "common/spin_lock.hpp"

namespace kagome::telemetry {

  using MessageHandle = std::size_t;

  /**
   * Message pool used to deduplicate and hold the data till async write
   * operations are in progress.
   *
   * The pool is designed to be extremely fast against data-copy operations.
   * That is why all the storage buffers are pre-allocated during the
   * construction.
   * All the copy operations are performed in a fastest manner via memcpy.
   *
   * Access operations to shared data where data race is possible are
   * synchronized via spin locks. Locking is performed for a smallest possible
   * set of lines of code. Spin locks let us avoid expensive context switches
   * and performance degradation.
   */
  class MessagePool {
   public:
    /**
     * Type for reference external reference counters.
     * Normally cannot be negative.
     * That sized and signed type is chosen to easily catch illegal cases
     * (negative and too hign numbers -> will more likely result in an
     * overflow).
     */
    using RefCount = int16_t;
    /**
     * Contruct the pool
     * @param entry_size_bytes - max size of a single record
     * @param entries_count - max amount of records to hold at the same time
     */
    MessagePool(std::size_t entry_size_bytes, std::size_t entries_count);

    /**
     * Put a message to the pool
     * @param message - the data, could be disposed immediately after returning
     * @param ref_count - initial reference counter value for the record
     * @return - handle to the record or std::nullopt when the pool is full
     *
     * Note: the record will be freed from the pool as soon as an handle-owner
     * calls the release method for the handle ref_count-many times
     */
    std::optional<MessageHandle> push(const std::string &message,
                                      int16_t ref_count);

    /**
     * Increase reference counter for the specified handle
     * @param handle - handle to the record
     *
     * Would be useful in case when many verbosity level would be developed.
     */
    RefCount add_ref(MessageHandle handle);

    /**
     * Decrement reference counter for the handled record.
     * @param handle - handle to the record
     *
     * The record would be disposed when counter decreases to zero.
     */
    RefCount release(MessageHandle handle);

    /**
     * Access the record by the handle.
     * @param handle - handle to the record
     * @return - boost::asio::buffer with the contents ready to be passed to
     * boost::asio::async_write call
     */
    boost::asio::mutable_buffer operator[](MessageHandle handle) const;

    /**
     * Reports a number of records the pool was initialized for.
     * @return pool capacity
     */
    std::size_t capacity() const;

   private:
    /// performs quick lookup for a free slot
    std::optional<MessageHandle> nextFreeSlot();

    struct Record {
      std::vector<uint8_t> data;
      std::size_t data_size = 0;
      std::size_t ref_count = 0;
    };

    const std::size_t entry_size_;
    const std::size_t entries_count_;
    std::vector<Record> pool_;
    std::unordered_set<std::size_t> free_slots_;
    common::spin_lock mutex_;
  };

}  // namespace kagome::telemetry

#endif  // KAGOME_MESSAGE_POOL_HPP
