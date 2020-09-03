/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LEVELDB_CURSOR_HPP
#define KAGOME_LEVELDB_CURSOR_HPP

#include <leveldb/iterator.h>
#include "storage/leveldb/leveldb.hpp"

namespace kagome::storage {

  /**
   * @brief Instance of cursor can be used as bidirectional iterator over
   * key-value bindings of the Map.
   */
  class LevelDB::Cursor : public BufferMapCursor {
   public:
    ~Cursor() override = default;

    explicit Cursor(std::shared_ptr<leveldb::Iterator> it);

    outcome::result<bool> seekFirst() override;

    outcome::result<bool> seek(const Buffer &key) override;

    outcome::result<bool> seekLast() override;

    bool isValid() const override;

    outcome::result<void> next() override;

    outcome::result<void> prev();

    boost::optional<Buffer> key() const override;

    boost::optional<Buffer> value() const override;

   private:
    std::shared_ptr<leveldb::Iterator> i_;
  };

}  // namespace kagome::storage

#endif  // KAGOME_LEVELDB_CURSOR_HPP
