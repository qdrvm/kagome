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

#include "storage/leveldb/leveldb_cursor.hpp"

#include "storage/leveldb/leveldb_util.hpp"

namespace kagome::storage {

  LevelDB::Cursor::Cursor(std::shared_ptr<leveldb::Iterator> it)
      : i_(std::move(it)) {}

  void LevelDB::Cursor::seekToFirst() {
    i_->SeekToFirst();
  }

  void LevelDB::Cursor::seek(const Buffer &key) {
    i_->Seek(make_slice(key));
  }

  void LevelDB::Cursor::seekToLast() {
    i_->SeekToLast();
  }

  bool LevelDB::Cursor::isValid() const {
    return i_->Valid();
  }

  void LevelDB::Cursor::next() {
    i_->Next();
  }

  void LevelDB::Cursor::prev() {
    i_->Prev();
  }

  Buffer LevelDB::Cursor::key() const {
    return make_buffer(i_->key());
  }

  Buffer LevelDB::Cursor::value() const {
    return make_buffer(i_->value());
  }

}  // namespace kagome::storage
