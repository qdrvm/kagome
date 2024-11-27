/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/rocksdb/rocksdb.hpp"
#include <rocksdb/db.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/table.h>
#include <qtils/outcome.hpp>

#include "filesystem/common.hpp"

#include "storage/buffer_map_types.hpp"
#include "storage/database_error.hpp"
#include "storage/rocksdb/rocksdb_batch.hpp"
#include "storage/rocksdb/rocksdb_cursor.hpp"
#include "storage/rocksdb/rocksdb_spaces.hpp"
#include "storage/rocksdb/rocksdb_util.hpp"
#include "utils/mkdirs.hpp"

namespace kagome::storage {
  namespace fs = std::filesystem;

  RocksDb::RocksDb() : logger_(log::createLogger("RocksDB", "storage")) {
    ro_.fill_cache = false;
  }

  RocksDb::~RocksDb() {
    for (auto *handle : column_family_handles_) {
      db_->DestroyColumnFamilyHandle(handle);
    }
    delete db_;
  }
  outcome::result<std::shared_ptr<RocksDb>> RocksDb::create(
      const filesystem::path &path,
      rocksdb::Options options,
      uint32_t memory_budget_mib,
      bool prevent_destruction,
      const std::unordered_map<std::string, int32_t> &column_ttl,
      bool enable_migration) {
    const auto no_db_presented = not fs::exists(path);
    OUTCOME_TRY(mkdirs(path));

    auto log = log::createLogger("RocksDB", "storage");
    auto absolute_path = fs::absolute(path);

    OUTCOME_TRY(createDirectory(absolute_path, log));

    const auto memory_budget = memory_budget_mib * 1024 * 1024;
    const auto trie_space_cache_size =
        static_cast<uint32_t>(memory_budget * 0.9);
    const uint32_t other_spaces_cache_size =
        (memory_budget - trie_space_cache_size) / (storage::Space::kTotal - 1);

    std::vector<rocksdb::ColumnFamilyDescriptor> column_family_descriptors;
    std::vector<int32_t> ttls;
    configureColumnFamilies(column_family_descriptors,
                            ttls,
                            column_ttl,
                            trie_space_cache_size,
                            other_spaces_cache_size,
                            log);

    std::vector<std::string> existing_families;
    auto res = rocksdb::DB::ListColumnFamilies(
        options, path.native(), &existing_families);
    if (!res.ok() && !res.IsPathNotFound()) {
      SL_ERROR(log,
               "Can't list column families in {}: {}",
               absolute_path.native(),
               res.ToString());
      return status_as_error(res);
    }

    options.create_missing_column_families = true;
    auto rocks_db = std::shared_ptr<RocksDb>(new RocksDb);
    const auto ttl_migrated_path = path.parent_path() / "ttl_migrated";
    const auto ttl_migrated_exists = fs::exists(ttl_migrated_path);

    if (no_db_presented or ttl_migrated_exists) {
      OUTCOME_TRY(openDatabaseWithTTL(options,
                                      path,
                                      column_family_descriptors,
                                      ttls,
                                      rocks_db,
                                      ttl_migrated_path,
                                      log));
      return rocks_db;
    }

    if (not enable_migration) {
      SL_ERROR(log,
               "Database migration is disabled, use older kagome version or "
               "run with migration enabling flag");
      return DatabaseError::IO_ERROR;
    }

    OUTCOME_TRY(migrateDatabase(options,
                                path,
                                column_family_descriptors,
                                ttls,
                                rocks_db,
                                ttl_migrated_path,
                                log));
    return rocks_db;
  }

  outcome::result<void> RocksDb::createDirectory(
      const std::filesystem::path &absolute_path, log::Logger &log) {
    std::error_code ec;
    if (not fs::create_directory(absolute_path.native(), ec) and ec.value()) {
      SL_ERROR(log,
               "Can't create directory {} for database: {}",
               absolute_path.native(),
               ec);
      return DatabaseError::IO_ERROR;
    }
    if (not fs::is_directory(absolute_path.native())) {
      SL_ERROR(log,
               "Can't open {} for database: is not a directory",
               absolute_path.native());
      return DatabaseError::IO_ERROR;
    }
    return outcome::success();
  }

  void RocksDb::configureColumnFamilies(
      std::vector<rocksdb::ColumnFamilyDescriptor> &column_family_descriptors,
      std::vector<int32_t> &ttls,
      const std::unordered_map<std::string, int32_t> &column_ttl,
      uint32_t trie_space_cache_size,
      uint32_t other_spaces_cache_size,
      log::Logger &log) {
    for (auto i = 0; i < Space::kTotal; ++i) {
      const auto space_name = spaceName(static_cast<Space>(i));
      auto ttl = 0;
      if (const auto it = column_ttl.find(space_name); it != column_ttl.end()) {
        ttl = it->second;
      }
      column_family_descriptors.emplace_back(
          space_name,
          configureColumn(i != Space::kTrieNode ? other_spaces_cache_size
                                                : trie_space_cache_size));
      ttls.push_back(ttl);
      SL_DEBUG(log, "Column family {} configured with TTL {}", space_name, ttl);
    }
  }

  outcome::result<void> RocksDb::openDatabaseWithTTL(
      const rocksdb::Options &options,
      const filesystem::path &path,
      const std::vector<rocksdb::ColumnFamilyDescriptor>
          &column_family_descriptors,
      const std::vector<int32_t> &ttls,
      std::shared_ptr<RocksDb> &rocks_db,
      const filesystem::path &ttl_migrated_path,
      log::Logger &log) {
    const auto status =
        rocksdb::DBWithTTL::Open(options,
                                 path.native(),
                                 column_family_descriptors,
                                 &rocks_db->column_family_handles_,
                                 &rocks_db->db_,
                                 ttls);
    if (not status.ok()) {
      SL_ERROR(log,
               "Can't open database in {}: {}",
               path.native(),
               status.ToString());
      return status_as_error(status);
    }
    for (auto *handle : rocks_db->column_family_handles_) {
      auto space = spaceByName(handle->GetName());
      BOOST_ASSERT(space.has_value());
      rocks_db->spaces_[*space] = std::make_shared<RocksDbSpace>(
          rocks_db->weak_from_this(), *space, rocks_db->logger_);
    }
    if (not fs::exists(ttl_migrated_path)) {
      std::ofstream file(ttl_migrated_path.native());
      if (not file) {
        SL_ERROR(log,
                 "Can't create file {} for database",
                 ttl_migrated_path.native());
        return DatabaseError::IO_ERROR;
      }
      file.close();
    }
    return outcome::success();
  }

  outcome::result<void> RocksDb::migrateDatabase(
      const rocksdb::Options &options,
      const filesystem::path &path,
      const std::vector<rocksdb::ColumnFamilyDescriptor>
          &column_family_descriptors,
      const std::vector<int32_t> &ttls,
      std::shared_ptr<RocksDb> &rocks_db,
      const filesystem::path &ttl_migrated_path,
      log::Logger &log) {
    rocksdb::DB *db_raw = nullptr;
    std::vector<rocksdb::ColumnFamilyHandle *> column_family_handles;
    auto status = rocksdb::DB::Open(options,
                                    path.native(),
                                    column_family_descriptors,
                                    &column_family_handles,
                                    &db_raw);
    std::shared_ptr<rocksdb::DB> db(db_raw);
    if (not status.ok()) {
      SL_ERROR(log,
               "Can't open old database in {}: {}",
               path.native(),
               status.ToString());
      return status_as_error(status);
    }
    auto defer_db =
        std::make_unique<DatabaseGuard>(db, column_family_handles, log);

    std::vector<rocksdb::ColumnFamilyHandle *> column_family_handles_with_ttl;
    const auto ttl_path = path.parent_path() / "db_ttl";
    std::error_code ec;
    fs::create_directories(ttl_path, ec);
    if (ec) {
      SL_ERROR(log,
               "Can't create directory {} for database: {}",
               ttl_path.native(),
               ec);
      return DatabaseError::IO_ERROR;
    }
    rocksdb::DBWithTTL *db_with_ttl_raw = nullptr;
    status = rocksdb::DBWithTTL::Open(options,
                                      ttl_path.native(),
                                      column_family_descriptors,
                                      &column_family_handles_with_ttl,
                                      &db_with_ttl_raw,
                                      ttls);
    if (not status.ok()) {
      SL_ERROR(log,
               "Can't open database in {}: {}",
               ttl_path.native(),
               status.ToString());
      return status_as_error(status);
    }
    std::shared_ptr<rocksdb::DBWithTTL> db_with_ttl(db_with_ttl_raw);
    auto defer_db_ttl = std::make_unique<DatabaseGuard>(
        db_with_ttl, column_family_handles_with_ttl, log);

    for (std::size_t i = 0; i < column_family_handles.size(); ++i) {
      const auto from_handle = column_family_handles[i];
      auto to_handle = column_family_handles_with_ttl[i];
      std::unique_ptr<rocksdb::Iterator> it(
          db->NewIterator(rocksdb::ReadOptions(), from_handle));
      for (it->SeekToFirst(); it->Valid(); it->Next()) {
        const auto &key = it->key();
        const auto &value = it->value();
        status =
            db_with_ttl->Put(rocksdb::WriteOptions(), to_handle, key, value);
        if (not status.ok()) {
          SL_ERROR(log, "Can't write to ttl database: {}", status.ToString());
          return status_as_error(status);
        }
      }
      if (not it->status().ok()) {
        return status_as_error(it->status());
      }
    }
    defer_db_ttl.reset();
    defer_db.reset();
    fs::remove_all(path, ec);
    if (ec) {
      SL_ERROR(log, "Can't remove old database in {}: {}", path.native(), ec);
      return DatabaseError::IO_ERROR;
    }
    fs::create_directories(path, ec);
    if (ec) {
      SL_ERROR(log,
               "Can't create directory {} for final database: {}",
               path.native(),
               ec);
      return DatabaseError::IO_ERROR;
    }
    fs::rename(ttl_path, path, ec);
    if (ec) {
      SL_ERROR(log,
               "Can't rename database from {} to {}: {}",
               ttl_path.native(),
               path.native(),
               ec);
      return DatabaseError::IO_ERROR;
    }
    status = rocksdb::DBWithTTL::Open(options,
                                      path.native(),
                                      column_family_descriptors,
                                      &rocks_db->column_family_handles_,
                                      &rocks_db->db_,
                                      ttls);
    if (not status.ok()) {
      SL_ERROR(log,
               "Can't open database in {}: {}",
               path.native(),
               status.ToString());
      return status_as_error(status);
    }
    std::ofstream file(ttl_migrated_path.native());
    if (not file) {
      SL_ERROR(
          log, "Can't create file {} for database", ttl_migrated_path.native());
      return DatabaseError::IO_ERROR;
    }
    file.close();
    return outcome::success();
  }

  std::shared_ptr<BufferBatchableStorage> RocksDb::getSpace(Space space) {
    auto it = spaces_.find(space);
    BOOST_ASSERT(it != spaces_.end());
    return it->second;
  }

  outcome::result<void> RocksDb::dropColumn(kagome::storage::Space space) {
    auto column = getCFHandle(space);
    auto check_status =
        [this](const rocksdb::Status &status) -> outcome::result<void> {
      if (!status.ok()) {
        logger_->error("DB operation failed: {}", status.ToString());
        return status_as_error(status);
      }
      return outcome::success();
    };
    OUTCOME_TRY(check_status(db_->DropColumnFamily(column)));
    OUTCOME_TRY(check_status(db_->DestroyColumnFamilyHandle(column)));
    rocksdb::ColumnFamilyHandle *new_handle{};
    OUTCOME_TRY(check_status(
        db_->CreateColumnFamily({}, spaceName(space), &new_handle)));
    column_family_handles_[static_cast<size_t>(space)] = new_handle;
    return outcome::success();
  }

  rocksdb::BlockBasedTableOptions RocksDb::tableOptionsConfiguration(
      uint32_t lru_cache_size_mib, uint32_t block_size_kib) {
    rocksdb::BlockBasedTableOptions table_options;
    table_options.format_version = 5;
    table_options.block_cache = rocksdb::NewLRUCache(
        static_cast<uint64_t>(lru_cache_size_mib) * 1024 * 1024);
    table_options.block_size = static_cast<size_t>(block_size_kib) * 1024;
    table_options.cache_index_and_filter_blocks = true;
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, false));
    return table_options;
  }

  rocksdb::ColumnFamilyHandle *RocksDb::getCFHandle(Space space) {
    BOOST_ASSERT_MSG(static_cast<size_t>(space) < column_family_handles_.size(),
                     "All spaces should have an associated column family");
    auto handle = column_family_handles_[static_cast<size_t>(space)];
    BOOST_ASSERT(handle != nullptr);
    return handle;
  }

  rocksdb::ColumnFamilyOptions RocksDb::configureColumn(
      uint32_t memory_budget) {
    rocksdb::ColumnFamilyOptions options;
    options.OptimizeLevelStyleCompaction(memory_budget);
    auto table_options = tableOptionsConfiguration();
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));
    return options;
  }

  std::unique_ptr<BufferSpacedBatch> RocksDb::createBatch() {
    return std::make_unique<RocksDbBatch>(shared_from_this(),
                                          getCFHandle(Space::kDefault));
  }

  RocksDb::DatabaseGuard::DatabaseGuard(
      std::shared_ptr<rocksdb::DB> db,
      std::vector<rocksdb::ColumnFamilyHandle *> column_family_handles,
      log::Logger log)
      : db_(std::move(db)),
        column_family_handles_(std::move(column_family_handles)),
        log_(std::move(log)) {}

  RocksDb::DatabaseGuard::DatabaseGuard(
      std::shared_ptr<rocksdb::DBWithTTL> db_ttl,
      std::vector<rocksdb::ColumnFamilyHandle *> column_family_handles,
      log::Logger log)
      : db_ttl_(std::move(db_ttl)),
        column_family_handles_(std::move(column_family_handles)),
        log_(std::move(log)) {}

  RocksDb::DatabaseGuard::~DatabaseGuard() {
    const auto clean = [this](auto db) {
      auto status = db->Flush(rocksdb::FlushOptions());
      if (not status.ok()) {
        SL_ERROR(log_, "Can't flush database: {}", status.ToString());
      }

      status = db->WaitForCompact(rocksdb::WaitForCompactOptions());
      if (not status.ok()) {
        SL_ERROR(log_,
                 "Can't wait for background compaction: {}",
                 status.ToString());
      }

      for (auto *handle : column_family_handles_) {
        db->DestroyColumnFamilyHandle(handle);
      }

      status = db->Close();
      if (not status.ok()) {
        SL_ERROR(log_, "Can't close database: {}", status.ToString());
      }
      db.reset();
    };
    if (db_) {
      clean(db_);
    } else if (db_ttl_) {
      clean(db_ttl_);
    }
  }

  RocksDbSpace::RocksDbSpace(std::weak_ptr<RocksDb> storage,
                             Space space,
                             log::Logger logger)
      : storage_{std::move(storage)},
        space_{space},
        logger_{std::move(logger)} {}

  std::optional<size_t> RocksDbSpace::byteSizeHint() const {
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

  std::unique_ptr<face::WriteBatch<Buffer, Buffer>> RocksDbSpace::batch() {
    auto rocks = storage_.lock();
    if (!rocks) {
      throw DatabaseError::STORAGE_GONE;
    }
    auto batch =
        std::make_unique<RocksDbBatch>(rocks, rocks->getCFHandle(space_));
    return batch;
  }

  std::unique_ptr<RocksDbSpace::Cursor> RocksDbSpace::cursor() {
    auto rocks = storage_.lock();
    if (!rocks) {
      throw DatabaseError::STORAGE_GONE;
    }
    auto it = std::unique_ptr<rocksdb::Iterator>(
        rocks->db_->NewIterator(rocks->ro_, rocks->getCFHandle(space_)));
    return std::make_unique<RocksDBCursor>(std::move(it));
  }

  outcome::result<bool> RocksDbSpace::contains(const BufferView &key) const {
    OUTCOME_TRY(rocks, use());
    std::string value;
    auto status = rocks->db_->Get(
        rocks->ro_, rocks->getCFHandle(space_), make_slice(key), &value);
    if (status.ok()) {
      return true;
    }

    if (status.IsNotFound()) {
      return false;
    }

    return status_as_error(status);
  }

  outcome::result<BufferOrView> RocksDbSpace::get(const BufferView &key) const {
    OUTCOME_TRY(rocks, use());
    std::string value;
    auto status = rocks->db_->Get(
        rocks->ro_, rocks->getCFHandle(space_), make_slice(key), &value);
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
    auto status = rocks->db_->Get(
        rocks->ro_, rocks->getCFHandle(space_), make_slice(key), &value);
    if (status.ok()) {
      auto buf = Buffer(
          reinterpret_cast<uint8_t *>(value.data()),                  // NOLINT
          reinterpret_cast<uint8_t *>(value.data()) + value.size());  // NOLINT
      return std::make_optional(BufferOrView(std::move(buf)));
    }

    if (status.IsNotFound()) {
      return std::nullopt;
    }

    return status_as_error(status);
  }

  outcome::result<void> RocksDbSpace::put(const BufferView &key,
                                          BufferOrView &&value) {
    OUTCOME_TRY(rocks, use());
    auto status = rocks->db_->Put(rocks->wo_,
                                  rocks->getCFHandle(space_),
                                  make_slice(key),
                                  make_slice(std::move(value)));
    if (status.ok()) {
      return outcome::success();
    }

    return status_as_error(status);
  }

  outcome::result<void> RocksDbSpace::remove(const BufferView &key) {
    OUTCOME_TRY(rocks, use());
    auto status = rocks->db_->Delete(
        rocks->wo_, rocks->getCFHandle(space_), make_slice(key));
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
          rocks->db_->NewIterator(rocks->ro_, rocks->getCFHandle(space_)));
      first.empty() ? begin->SeekToFirst() : begin->Seek(make_slice(first));
      auto bk = begin->key();
      std::unique_ptr<rocksdb::Iterator> end(
          rocks->db_->NewIterator(rocks->ro_, rocks->getCFHandle(space_)));
      last.empty() ? end->SeekToLast() : end->Seek(make_slice(last));
      auto ek = end->key();
      rocksdb::CompactRangeOptions options;
      rocks->db_->CompactRange(options, rocks->getCFHandle(space_), &bk, &ek);
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
