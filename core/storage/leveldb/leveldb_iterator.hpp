/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LEVELDB_ITERATOR_HPP
#define KAGOME_LEVELDB_ITERATOR_HPP

#include <leveldb/iterator.h>
#include "storage/leveldb/leveldb.hpp"

namespace kagome::storage {

  class LevelDB::Iterator : public BufferMapIterator {
   public:
    ~Iterator() override = default;

    explicit Iterator(std::shared_ptr<leveldb::Iterator> it);

    /**
     * @brief Same as std::begin(...);
     */
    void seekToFirst() override;

    /**
     * @brief Find given key and seek iterator to this key.
     */
    void seek(const Buffer &key) override;

    /**
     * @brief Same as std::rbegin(...);, e.g. points to the last valid element
     */
    void seekToLast() override;

    /**
     * @brief Is iterator valid?
     * @return true if iterator points to the element of map, false otherwise
     */
    bool isValid() override;

    /**
     * @brief Make step forward.
     */
    void next() override;

    /**
     * @brief Make step backwards.
     */
    void prev() override;

    /**
     * @brief Getter for key.
     * @return key
     */
    Buffer key() const override;

    /**
     * @brief Getter for value.
     * @return value
     */
    Buffer value() const override;

   private:
    std::shared_ptr<leveldb::Iterator> i_;
  };

}  // namespace kagome::storage

#endif  // KAGOME_LEVELDB_ITERATOR_HPP
