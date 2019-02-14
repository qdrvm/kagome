/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RECORD_STORE_HPP
#define KAGOME_RECORD_STORE_HPP

#include <vector>

#include "libp2p/multi/multihash.hpp"
#include "libp2p/store/record.hpp"

namespace libp2p::store {
  /**
   * Distributed record store
   */
  class RecordStore {
    /**
     * Get records from the store
     * @param key of the records
     * @return records
     */
    virtual std::vector<Record> get(const multi::Multihash &key) const = 0;

    /**
     * Put a record to the store
     * @param key, under which that record will be stored
     * @param value_hash - multihash of signature of MerkleDAG record
     */
    virtual void put(const multi::Multihash &key,
                     const multi::Multihash &value_hash) = 0;
  };
}  // namespace libp2p::store

#endif  // KAGOME_RECORD_STORE_HPP
