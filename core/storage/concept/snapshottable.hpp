/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SNAPSHOTTABLE_HPP
#define KAGOME_SNAPSHOTTABLE_HPP

namespace kagome::storage::concept {

  /**
   * @brief An abstraction over a storage, which can produce readable snapshots.
   * Snapshots can be used to iterate over database with given state, which is
   * changed over time.
   * @tparam snapshot_t type of snapshot
   */
  template <typename snapshot_t>
  struct Snapshottable {
    virtual ~Snapshottable() = default;

    /**
     * @brief Acquire a read snapshot for a given storage
     */
    virtual snapshot_t getSnapshot() = 0;

    /**
     * @brief Release a read snapshot for a given storage
     */
    virtual void releaseSnapshot(snapshot_t &t) = 0;
  };

}  // namespace kagome::storage::concept

#endif  // KAGOME_SNAPSHOTTABLE_HPP
