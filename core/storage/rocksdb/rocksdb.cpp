/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/rocksdb/rocksdb.hpp"

#include <boost/filesystem.hpp>

#include "filesystem/directories.hpp"
#include "storage/database_error.hpp"
#include "storage/rocksdb/rocksdb_batch.hpp"
#include "storage/rocksdb/rocksdb_cursor.hpp"
#include "storage/rocksdb/rocksdb_util.hpp"

namespace kagome::storage {
  namespace fs = boost::filesystem;

  RocksDB::RocksDB(bool prevent_destruction)
      : prevent_destruction_(prevent_destruction) {
    ro_.fill_cache = false;
  }

  RocksDB::~RocksDB() {
    if (prevent_destruction_) {
      /*
       * The following is a dirty workaround for a bug of rocksdb impl.
       * Luckily, this happens when the whole app destroys, that is why
       * the intentional memory leak would not affect normal operation.
       *
       * The hack is used to mitigate an issue of recovery mode run.
       */
      std::ignore = db_.release();
    }
  }

  outcome::result<std::unique_ptr<RocksDB>> RocksDB::create(
      const boost::filesystem::path &path,
      rocksdb::Options options,
      bool prevent_destruction) {
    if (!filesystem::createDirectoryRecursive(path)) {
      return DatabaseError::DB_PATH_NOT_CREATED;
    }

    rocksdb::DB *db = nullptr;

    auto log = log::createLogger("RocksDB", "storage");
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

    auto status = rocksdb::DB::Open(options, path.native(), &db);
    if (status.ok()) {
      std::unique_ptr<RocksDB> l{new RocksDB(prevent_destruction)};
      l->db_ = std::unique_ptr<rocksdb::DB>(db);
      l->logger_ = std::move(log);
      return l;
    }

    SL_ERROR(log,
             "Can't open database in {}: {}",
             absolute_path.native(),
             status.ToString());

    return status_as_error(status);
  }

  std::unique_ptr<BufferBatch> RocksDB::batch() {
    return std::make_unique<Batch>(*this);
  }

  size_t RocksDB::size() const {
    size_t usage_bytes = 0;
    if (db_) {
      std::string usage;
      bool result = db_->GetProperty("rocksdb.cur-size-all-mem-tables", &usage);
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

  std::unique_ptr<RocksDB::Cursor> RocksDB::cursor() {
    auto it = std::unique_ptr<rocksdb::Iterator>(db_->NewIterator(ro_));
    return std::make_unique<RocksDBCursor>(std::move(it));
  }

  outcome::result<bool> RocksDB::contains(const BufferView &key) const {
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

  bool RocksDB::empty() const {
    auto it = std::unique_ptr<rocksdb::Iterator>(db_->NewIterator(ro_));
    it->SeekToFirst();
    return it->Valid();
  }

  outcome::result<Buffer> RocksDB::load(const BufferView &key) const {
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

  outcome::result<std::optional<Buffer>> RocksDB::tryLoad(
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

  outcome::result<void> RocksDB::put(const BufferView &key,
                                     const Buffer &value) {
    auto status = db_->Put(wo_, make_slice(key), make_slice(value));
    if (status.ok()) {
      return outcome::success();
    }

    return status_as_error(status);
  }

  outcome::result<void> RocksDB::put(const BufferView &key, Buffer &&value) {
    Buffer copy(std::move(value));
    return put(key, copy);
  }

  outcome::result<void> RocksDB::remove(const BufferView &key) {
    auto status = db_->Delete(wo_, make_slice(key));
    if (status.ok()) {
      return outcome::success();
    }

    return status_as_error(status);
  }

  void RocksDB::compact(const Buffer &first, const Buffer &last) {
    if (db_) {
      auto *begin = db_->NewIterator(ro_);
      first.empty() ? begin->SeekToFirst() : begin->Seek(make_slice(first));
      auto bk = begin->key();
      auto *end = db_->NewIterator(ro_);
      last.empty() ? end->SeekToLast() : end->Seek(make_slice(last));
      auto ek = end->key();
      rocksdb::CompactRangeOptions options;
      db_->CompactRange(options, &bk, &ek);
      delete begin;
      delete end;
    }
  }
}  // namespace kagome::storage
