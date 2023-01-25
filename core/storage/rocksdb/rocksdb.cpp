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
#include "storage/rocksdb/rocksdb_spaces.hpp"
#include "storage/rocksdb/rocksdb_util.hpp"

namespace kagome::storage {
  namespace fs = boost::filesystem;

  RocksDb::RocksDb(bool prevent_destruction,
                   std::vector<ColumnFamilyHandle> columns)
      : prevent_destruction_{prevent_destruction},
        cf_handles_{std::move(columns)} {
    ro_.fill_cache = false;
  }

  RocksDb::~RocksDb() {
    for (auto &cf : cf_handles_) {
      if (nullptr != cf.handle) {
        db_->DestroyColumnFamilyHandle(cf.handle);
      }
    }
    // a better solution is required
    if (prevent_destruction_) {
      db_.release();
    }
  }

  outcome::result<std::shared_ptr<RocksDb>> RocksDb::create(
      const filesystem::path &path,
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
                 ec);
      return DatabaseError::IO_ERROR;
    }
    if (not fs::is_directory(absolute_path.native())) {
      log->error("Can't open {} for database: is not a directory",
                 absolute_path.native());
      return DatabaseError::IO_ERROR;
    }

    std::vector<ColumnFamilyHandle> column_family_handles;
    std::vector<rocksdb::ColumnFamilyDescriptor> column_family_descriptors;
    for (auto i = 0; i < Space::kTotal; ++i) {
      auto name = spaceName(static_cast<Space>(i));
      column_family_handles.emplace_back(ColumnFamilyHandle{name, nullptr});
      column_family_descriptors.emplace_back(
          rocksdb::ColumnFamilyDescriptor{name, {}});
    }
    std::vector<rocksdb::ColumnFamilyHandle *> handles;
    options.create_missing_column_families = true;
    auto status = rocksdb::DB::Open(
        options, path.native(), column_family_descriptors, &handles, &db);
    if (status.ok()) {
      assert(handles.size() == Space::kTotal);
      for (size_t i = 0; i < handles.size(); ++i) {
        assert(handles[i]->GetName() == column_family_handles[i].name);
        column_family_handles[i].handle = handles[i];
      }

      std::unique_ptr<RocksDb> l{
          new RocksDb(prevent_destruction, std::move(column_family_handles))};
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

  std::shared_ptr<BufferStorage> RocksDb::getSpace(Space space) {
    if (spaces_.contains(space)) {
      return spaces_[space];
    }
    auto space_name = spaceName(space);
    auto column = std::find_if(cf_handles_.begin(),
                               cf_handles_.end(),
                               [&space_name](const ColumnFamilyHandle &handle) {
                                 return handle.name == space_name;
                               });
    if (cf_handles_.end() == column) {
      throw DatabaseError::INVALID_ARGUMENT;
    }
    auto space_ptr =
        std::make_shared<RocksDbSpace>(weak_from_this(), *column, logger_);
    spaces_[space] = space_ptr;
    return space_ptr;
  }

  RocksDbSpace::RocksDbSpace(std::weak_ptr<RocksDb> storage,
                             RocksDb::ColumnFamilyHandle column,
                             log::Logger logger)
      : storage_{std::move(storage)},
        column_{std::move(column)},
        logger_{std::move(logger)} {}

  std::unique_ptr<BufferBatch> RocksDbSpace::batch() {
    return std::make_unique<RocksDbBatch>(*this);
  }

  size_t RocksDbSpace::size() const {
    auto rocks = storage_.lock();
    if (!rocks) {
      return 0;
    }
    size_t usage_bytes = 0;
    if (rocks->db_) {
      std::string usage;
      bool result =
          rocks->db_->GetProperty("rocksdb.cur-size-all-mem-tables", &usage);
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

  std::unique_ptr<RocksDbSpace::Cursor> RocksDbSpace::cursor() {
    auto rocks = storage_.lock();
    if (!rocks) {
      throw DatabaseError::STORAGE_GONE;
    }
    auto it = std::unique_ptr<rocksdb::Iterator>(
        rocks->db_->NewIterator(rocks->ro_, column_.handle));
    return std::make_unique<RocksDBCursor>(std::move(it));
  }

  outcome::result<bool> RocksDbSpace::contains(const BufferView &key) const {
    OUTCOME_TRY(rocks, use());
    std::string value;
    auto status =
        rocks->db_->Get(rocks->ro_, column_.handle, make_slice(key), &value);
    if (status.ok()) {
      return true;
    }

    if (status.IsNotFound()) {
      return false;
    }

    return status_as_error(status);
  }

  bool RocksDbSpace::empty() const {
    auto rocks = storage_.lock();
    if (!rocks) {
      return true;
    }
    auto it = std::unique_ptr<rocksdb::Iterator>(
        rocks->db_->NewIterator(rocks->ro_, column_.handle));
    it->SeekToFirst();
    return it->Valid();
  }

  outcome::result<BufferOrView> RocksDbSpace::get(const BufferView &key) const {
    OUTCOME_TRY(rocks, use());
    std::string value;
    auto status =
        rocks->db_->Get(rocks->ro_, column_.handle, make_slice(key), &value);
    if (status.ok()) {
      // cannot move string content to a buffer
      return Buffer(
          reinterpret_cast<uint8_t *>(value.data()),                  // NOLINT
          reinterpret_cast<uint8_t *>(value.data()) + value.size());  // NOLINT
    }
    return status_as_error(status);
  }

  outcome::result<std::optional<BufferOrView>> RocksDbSpace::tryGet(
      const BufferView &key) const {
    OUTCOME_TRY(rocks, use());
    std::string value;
    auto status =
        rocks->db_->Get(rocks->ro_, column_.handle, make_slice(key), &value);
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

  outcome::result<void> RocksDbSpace::put(const BufferView &key,
                                          BufferOrView &&value) {
    OUTCOME_TRY(rocks, use());
    auto status = rocks->db_->Put(
        rocks->wo_, column_.handle, make_slice(key), make_slice(value));
    if (status.ok()) {
      return outcome::success();
    }

    return status_as_error(status);
  }

  outcome::result<void> RocksDbSpace::remove(const BufferView &key) {
    OUTCOME_TRY(rocks, use());
    auto status =
        rocks->db_->Delete(rocks->wo_, column_.handle, make_slice(key));
    if (status.ok()) {
      return outcome::success();
    }

    return status_as_error(status);
  }

  void RocksDbSpace::compact(const Buffer &first, const Buffer &last) {
    auto rocks = storage_.lock();
    if (!rocks) {
      return;
    }
    if (rocks->db_) {
      std::unique_ptr<rocksdb::Iterator> begin(
          rocks->db_->NewIterator(rocks->ro_, column_.handle));
      first.empty() ? begin->SeekToFirst() : begin->Seek(make_slice(first));
      auto bk = begin->key();
      std::unique_ptr<rocksdb::Iterator> end(
          rocks->db_->NewIterator(rocks->ro_, column_.handle));
      last.empty() ? end->SeekToLast() : end->Seek(make_slice(last));
      auto ek = end->key();
      rocksdb::CompactRangeOptions options;
      rocks->db_->CompactRange(options, column_.handle, &bk, &ek);
    }
  }

  outcome::result<std::shared_ptr<RocksDb>> RocksDbSpace::use() const {
    auto rocks = storage_.lock();
    if (!rocks) {
      return DatabaseError::STORAGE_GONE;
    }
    return rocks;
  }
}  // namespace kagome::storage
