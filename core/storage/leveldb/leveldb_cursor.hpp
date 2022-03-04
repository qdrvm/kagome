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
  class LevelDBCursor : public BufferStorageCursor {
   public:
    ~LevelDBCursor() override = default;

    explicit LevelDBCursor(std::shared_ptr<leveldb::Iterator> it);

    outcome::result<bool> seekFirst() override;

    outcome::result<bool> seek(const BufferView &key) override;

    outcome::result<bool> seekLast() override;

    bool isValid() const override;

    outcome::result<void> next() override;

    outcome::result<void> prev();

    std::optional<Buffer> key() const override;

    std::optional<Buffer> value() const override;

   private:
    std::shared_ptr<leveldb::Iterator> i_;
  };

}  // namespace kagome::storage

#endif  // KAGOME_LEVELDB_CURSOR_HPP
