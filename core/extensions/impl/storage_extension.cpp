/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/storage_extension.hpp"

#include <forward_list>

#include "primitives/block_id.hpp"
#include "storage/trie/impl/ordered_trie_hash.hpp"

using kagome::common::Buffer;

namespace kagome::extensions {
  StorageExtension::StorageExtension(
      std::shared_ptr<storage::trie::TrieDb> db,
      std::shared_ptr<runtime::WasmMemory> memory)
      : db_(std::move(db)),
        memory_(std::move(memory)),
        logger_{common::createLogger(kDefaultLoggerTag)} {
    BOOST_ASSERT_MSG(db_ != nullptr, "db is nullptr");
    BOOST_ASSERT_MSG(memory_ != nullptr, "memory is nullptr");
  }

  // -------------------------Data storage--------------------------

  void StorageExtension::ext_clear_prefix(runtime::WasmPointer prefix_data,
                                          runtime::SizeType prefix_length) {
    auto prefix = memory_->loadN(prefix_data, prefix_length);
    logger_->debug("Clear prefix: {}", prefix.toHex());
    auto res = db_->clearPrefix(prefix);
    if (not res) {
      //      logger_->error("ext_clear_prefix failed: {}",
      //      res.error().message());
    }
  }

  void StorageExtension::ext_clear_storage(runtime::WasmPointer key_data,
                                           runtime::SizeType key_length) {
    auto key = memory_->loadN(key_data, key_length);
    logger_->debug("Clear storage: {}", key.toHex());
    auto del_result = db_->remove(key);
    if (not del_result) {
      //      logger_->warn(
      //          "ext_clear_storage did not delete key {} from trie db with
      //          reason: "
      //          "{}",
      //          key_data,
      //          del_result.error().message());
    }
  }

  runtime::SizeType StorageExtension::ext_exists_storage(
      runtime::WasmPointer key_data, runtime::SizeType key_length) const {
    auto key = memory_->loadN(key_data, key_length);
    return db_->contains(key) ? 1 : 0;
  }

  runtime::WasmPointer StorageExtension::ext_get_allocated_storage(
      runtime::WasmPointer key_data,
      runtime::SizeType key_length,
      runtime::WasmPointer len_ptr) {
    auto key = memory_->loadN(key_data, key_length);
    auto data = db_->get(key);
    const auto length = data.has_value() ? data.value().size()
                                         : runtime::WasmMemory::kMaxMemorySize;
    memory_->store32(len_ptr, length);

    if (not data) {
      return 0;
    }
    //    if (not data.value().empty())
    //      logger_->debug("ext_get_allocated_storage. Key hex: {} Value hex
    //      {}",
    //                     key.toHex(),
    //                     data.value().toHex());

    auto data_ptr = memory_->allocate(length);

    if (data_ptr != 0) {
      memory_->storeBuffer(data_ptr, data.value());
    } else {
      //      logger_->error(
      //          "ext_get_allocated_storage failed: memory could not allocate
      //          enough " "memory");
    }
    return data_ptr;
  }

  runtime::SizeType StorageExtension::ext_get_storage_into(
      runtime::WasmPointer key_data,
      runtime::SizeType key_length,
      runtime::WasmPointer value_data,
      runtime::SizeType value_length,
      runtime::SizeType value_offset) {
    auto key = memory_->loadN(key_data, key_length);
    auto data = get(key, value_offset, value_length);
    if (not data) {
      //      logger_->debug("ext_get_storage_into. Val by key {} not found",
      //                     key.toHex());
      return runtime::WasmMemory::kMaxMemorySize;
    }
    if (not data.value().empty()) {
      //      logger_->debug("ext_get_storage_into. Key hex: {} , Value hex {}",
      //                     key.toHex(),
      //                     data.value().toHex());
    } else {
      //      logger_->debug("ext_get_storage_into. Key hex: {} Value: empty",
      //                     key.toHex());
    }
    memory_->storeBuffer(value_data, data.value());
    return data.value().size();
  }

  void StorageExtension::ext_set_storage(const runtime::WasmPointer key_data,
                                         runtime::SizeType key_length,
                                         const runtime::WasmPointer value_data,
                                         runtime::SizeType value_length) {
    auto key = memory_->loadN(key_data, key_length);
    auto value = memory_->loadN(value_data, value_length);

    if (value.toHex().size() < 500) {
      logger_->debug("Set storage.  Key hex: {} Value hex {}",
                     //          key.data(),
                     key.toHex(),
                     //          value.data(),
                     value.toHex());
    } else {
      logger_->debug("Set storage. Key hex: {} Value is too big to display",
                     //          key.data(),
                     key.toHex());
    }

    auto put_result = db_->put(key, value);
    if (not put_result) {
      //      logger_->error(
      //          "ext_set_storage failed, due to fail in trie db with reason:
      //          {}", put_result.error().message());
    }
  }

  // -------------------------Trie operations--------------------------

  void StorageExtension::ext_blake2_256_enumerated_trie_root(
      runtime::WasmPointer values_data,
      runtime::WasmPointer lengths_data,
      runtime::SizeType values_num,
      runtime::WasmPointer result) {
    std::vector<uint32_t> lengths(values_num);
    for (size_t i = 0; i < values_num; i++) {
      lengths.at(i) = memory_->load32u(lengths_data + i * 4);
    }
    std::forward_list<Buffer> values(values_num);
    uint32_t offset = 0;
    auto curr_value = values.begin();
    for (size_t i = 0; i < values_num; i++, curr_value++) {
      *curr_value = memory_->loadN(values_data + offset, lengths.at(i));
      offset += lengths.at(i);
    }
    auto ordered_hash =
        storage::trie::calculateOrderedTrieHash(values.begin(), values.end());
    if (ordered_hash.has_value()) {
      memory_->storeBuffer(result, ordered_hash.value());
      logger_->warn("sssssssssssssssssssssssss Enumerated Root is: {}",
                    ordered_hash.value().toHex());
    } else {
      logger_->error(
          "ext_blake2_256_enumerated_trie_root resulted with an error: {}",
          ordered_hash.error().message());
    }
  }

  runtime::SizeType StorageExtension::ext_storage_changes_root(
      runtime::WasmPointer parent_hash_data,
      runtime::SizeType parent_hash_len,
      runtime::WasmPointer result) {
    // TODO (kamilsa): PRE-95 Implement ext_storage_changes_root, 03.04.2019.
    //    auto parent_hash = memory_->loadN(parent_hash_data, parent_hash_len);
    primitives::BlockHash result_hash;
    result_hash[primitives::BlockHash::size() - 1] = 1;
    common::Buffer result_buf(result_hash);
    memory_->storeBuffer(result, result_buf);
    logger_->error("ext_storage_changes_root unimplemented, assume no changes");
    return 1;
  }

  void StorageExtension::ext_storage_root(runtime::WasmPointer result) const {
    const auto &root = db_->getRootHash();
    logger_->warn("sssssssssssssssssssssssss Root is: {}", root.toHex());
    memory_->storeBuffer(result, root);
  }

  outcome::result<common::Buffer> StorageExtension::get(
      const common::Buffer &key,
      runtime::SizeType offset,
      runtime::SizeType max_length) const {
    OUTCOME_TRY(data, db_->get(key));

    const auto data_length =
        std::min<runtime::SizeType>(max_length, data.size() - offset);

    return common::Buffer(std::vector<uint8_t>(
        data.begin() + offset, data.begin() + offset + data_length));
  }
}  // namespace kagome::extensions
