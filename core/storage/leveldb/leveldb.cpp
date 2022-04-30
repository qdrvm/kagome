/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/leveldb/leveldb.hpp"

#include <boost/filesystem.hpp>
#include <iostream>
#include <utility>

#include "filesystem/common.hpp"
#include "filesystem/directories.hpp"
#include "storage/leveldb/leveldb_batch.hpp"
#include "storage/leveldb/leveldb_cursor.hpp"
#include "storage/leveldb/leveldb_util.hpp"

namespace kagome::storage {
  namespace fs = boost::filesystem;

  outcome::result<std::unique_ptr<LevelDB>> LevelDB::create(
      const filesystem::path &path, leveldb::Options options) {
    if (!filesystem::createDirectoryRecursive(path))
      return DatabaseError::DB_PATH_NOT_CREATED;

    leveldb::DB *db = nullptr;

    auto log = log::createLogger("LevelDb", "storage");

    auto absolute_path = fs::absolute(path, fs::current_path());

    boost::system::error_code ec;
    if (not fs::create_directory(absolute_path.native(), ec) and ec.value()) {
      log->error("Can't create directory {} for database: {}",
                 absolute_path.native(),
                 ec.message());
      return DatabaseError::IO_ERROR;
    }
    if (not fs::is_directory(absolute_path.native())) {
      log->error("Can't open {} for database: is not a directory",
                 absolute_path.native());
      return DatabaseError::IO_ERROR;
    }

    auto status = leveldb::DB::Open(options, path.native(), &db);
    if (status.ok()) {
      std::unique_ptr<LevelDB> l{new LevelDB{}};
      l->db_ = std::unique_ptr<leveldb::DB>(db);
      l->logger_ = std::move(log);
      return l;
    }

    log->error("Can't open database in {}: {}",
               absolute_path.native(),
               status.ToString());
    return status_as_error(status);
  }

  std::unique_ptr<LevelDB::Cursor> LevelDB::cursor() {
    auto it = std::unique_ptr<leveldb::Iterator>(db_->NewIterator(ro_));
    return std::make_unique<LevelDBCursor>(std::move(it));
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

  outcome::result<Buffer> LevelDB::load(const BufferView &key) const {
    std::string value;
    auto status = db_->Get(ro_, make_slice(key), &value);
    if (status.ok()) {
      // cannot move string content to a buffer
      return Buffer(
          reinterpret_cast<uint8_t *>(value.data()),                  // NOLINT
          reinterpret_cast<uint8_t *>(value.data()) + value.size());  // NOLINT
    }
    return status_as_error(status);
  }

  outcome::result<std::optional<Buffer>> LevelDB::tryLoad(
      const BufferView &key) const {
    std::string value;
    auto status = db_->Get(ro_, make_slice(key), &value);
    if (status.ok()) {
      return std::make_optional(Buffer(
          reinterpret_cast<uint8_t *>(value.data()),                   // NOLINT
          reinterpret_cast<uint8_t *>(value.data()) + value.size()));  // NOLINT
    }

    if (status.IsNotFound()) {
      return std::nullopt;
    }

    return status_as_error(status);
  }

  outcome::result<bool> LevelDB::contains(const BufferView &key) const {
    std::string value;
    auto status = db_->Get(ro_, make_slice(key), &value);
    if (status.ok()) {
      return true;
    }

    if (status.IsNotFound()) {
      return false;
    }

    return status_as_error(status);
  }

  bool LevelDB::empty() const {
    auto it = std::unique_ptr<leveldb::Iterator>(db_->NewIterator(ro_));
    it->SeekToFirst();
    return it->Valid();
  }

  outcome::result<void> LevelDB::put(const BufferView &key,
                                     const Buffer &value) {
    auto status = db_->Put(wo_, make_slice(key), make_slice(value));
    if (status.ok()) {
      return outcome::success();
    }

    return status_as_error(status);
  }

  outcome::result<void> LevelDB::put(const BufferView &key, Buffer &&value) {
    Buffer copy(std::move(value));
    return put(key, copy);
  }

  outcome::result<void> LevelDB::remove(const BufferView &key) {
    auto status = db_->Delete(wo_, make_slice(key));
    if (status.ok()) {
      return outcome::success();
    }

    return status_as_error(status);
  }

  void LevelDB::compact(const Buffer &first, const Buffer &last) {
    if (db_) {
      auto *begin = db_->NewIterator(ro_);
      first.empty() ? begin->SeekToFirst() : begin->Seek(make_slice(first));
      auto bk = begin->key();
      auto *end = db_->NewIterator(ro_);
      last.empty() ? end->SeekToLast() : end->Seek(make_slice(last));
      auto ek = end->key();
      db_->CompactRange(&bk, &ek);
      delete begin;
      delete end;
    }
  }

  size_t LevelDB::size() const {
    size_t usage_bytes = 0;
    if (db_) {
      std::string usage;
      bool result =
          db_->GetProperty("leveldb.approximate-memory-usage", &usage);
      if (result) {
        try {
          usage_bytes = std::stoul(usage);
        } catch (...) {
          logger_->error("Unable to parse memory usage value");
        }
      } else {
        logger_->error("Unable to retrieve memory usage value");
      }
    }
    return usage_bytes;
  }

}  // namespace kagome::storage
