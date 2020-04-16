/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "storage/leveldb/leveldb_batch.hpp"

#include "storage/leveldb/leveldb_util.hpp"

namespace kagome::storage {

  LevelDB::Batch::Batch(LevelDB &db) : db_(db) {}

  outcome::result<void> LevelDB::Batch::put(const Buffer &key,
                                            const Buffer &value) {
    batch_.Put(make_slice(key), make_slice(value));
    return outcome::success();
  }

  outcome::result<void> LevelDB::Batch::put(const Buffer &key,
                                            Buffer &&value) {
    return put(key, static_cast<const Buffer&>(value));
  }

  outcome::result<void> LevelDB::Batch::remove(const Buffer &key) {
    batch_.Delete(make_slice(key));
    return outcome::success();
  }

  outcome::result<void> LevelDB::Batch::commit() {
    auto status = db_.db_->Write(db_.wo_, &batch_);
    if (status.ok()) {
      return outcome::success();
    }

    return error_as_result<void>(status, db_.logger_);
  }

  void LevelDB::Batch::clear() {
    batch_.Clear();
  }

}  // namespace kagome::storage
