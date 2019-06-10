/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utility>

#include "storage/leveldb/leveldb.hpp"
#include "storage/leveldb/leveldb_batch.hpp"
#include "storage/leveldb/leveldb_cursor.hpp"
#include "storage/leveldb/leveldb_util.hpp"

namespace kagome::storage {

  outcome::result<std::unique_ptr<LevelDB>> LevelDB::create(
      std::string_view path, leveldb::Options options, common::Logger logger) {
    leveldb::DB *db = nullptr;
    auto status = leveldb::DB::Open(options, path.data(), &db);
    if (status.ok()) {
      auto l = std::make_unique<LevelDB>();
      l->db_ = std::unique_ptr<leveldb::DB>(db);
      l->logger_ = std::move(logger);
      return l;
    }

    return error_as_result<std::unique_ptr<LevelDB>>(status, logger);
  }

  std::unique_ptr<BufferMapCursor> LevelDB::cursor() {
    auto it = std::unique_ptr<leveldb::Iterator>(db_->NewIterator(ro_));
    return std::make_unique<Cursor>(std::move(it));
  }

  std::unique_ptr<BufferBatch> LevelDB::batch() {
    return std::make_unique<Batch>(*this);
  }

  void LevelDB::setReadOptions(leveldb::ReadOptions ro) {
    ro_ = ro;
  }

  void LevelDB::setWriteOptions(leveldb::WriteOptions wo) {
    wo_ = wo;
  }

  outcome::result<Buffer> LevelDB::get(const Buffer &key) const {
    std::string value;
    auto status = db_->Get(ro_, make_slice(key), &value);
    if (status.ok()) {
      // FIXME: is it possible to avoid copying string -> Buffer?
      return Buffer{}.put(value);
    }

    return error_as_result<Buffer>(status, logger_);
  }

  bool LevelDB::contains(const Buffer &key) const {
    // here we interpret all kinds of errors as "not found".
    // is there a better way?
    return get(key).has_value();
  }

  outcome::result<void> LevelDB::put(const Buffer &key, const Buffer &value) {
    auto status = db_->Put(wo_, make_slice(key), make_slice(value));
    if (status.ok()) {
      return outcome::success();
    }

    return error_as_result<void>(status, logger_);
  }

  outcome::result<void> LevelDB::put(const Buffer &key, Buffer &&value) {
    Buffer copy(std::move(value));
    return put(key, copy);
  }

  outcome::result<void> LevelDB::remove(const Buffer &key) {
    auto status = db_->Delete(wo_, make_slice(key));
    if (status.ok()) {
      return outcome::success();
    }

    return error_as_result<void>(status, logger_);
  }

}  // namespace kagome::storage
