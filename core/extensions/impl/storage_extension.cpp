/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/storage_extension.hpp"

namespace kagome::extensions {
  StorageExtension::StorageExtension(
      std::shared_ptr<storage::merkle::TrieDb> db,
      std::shared_ptr<runtime::WasmMemory> memory,
      std::shared_ptr<storage::merkle::Codec> codec, common::Logger logger)
      : db_(std::move(db)),
        memory_(std::move(memory)),
        codec_(std::move(codec)),
        logger_(std::move(logger)) {}

  // -------------------------Data storage--------------------------

  void StorageExtension::ext_clear_prefix(runtime::WasmPointer prefix_data,
                                          runtime::SizeType prefix_length) {
    auto prefix = memory_->loadN(prefix_data, prefix_length);
    db_->clearPrefix(prefix);
  }

  void StorageExtension::ext_clear_storage(runtime::WasmPointer key_data,
                                           runtime::SizeType key_length) {
    auto key = memory_->loadN(key_data, key_length);
    auto del_result = db_->del(key);
    if (not del_result) {
      logger_->warn(
          "ext_clear_storage did not delete key {} from trie db with reason: "
          "{}",
          key_data, del_result.error().message());
    }
  }

  runtime::SizeType StorageExtension::ext_exists_storage(
      runtime::WasmPointer key_data, runtime::SizeType key_length) {
    auto key = memory_->loadN(key_data, key_length);
    return db_->contains(key) ? 1 : 0;
  }

  runtime::WasmPointer StorageExtension::ext_get_allocated_storage(
      runtime::WasmPointer key_data, runtime::SizeType key_length,
      runtime::WasmPointer len_ptr) {
    auto key = memory_->loadN(key_data, key_length);
    auto data = db_->get(key);
    const auto length = data.has_value() ? data.value().size()
                                         : runtime::WasmMemory::kMaxMemorySize;
    memory_->store32(len_ptr, length);

    if (not data) {
      return 0;
    }

    auto data_ptr = memory_->allocate(length);

    if (data_ptr != -1) {
      memory_->storeBuffer(data_ptr, data.value());
    }
    return data_ptr;
  }

  runtime::SizeType StorageExtension::ext_get_storage_into(
      runtime::WasmPointer key_data, runtime::SizeType key_length,
      runtime::WasmPointer value_data, runtime::SizeType value_length,
      runtime::SizeType value_offset) {
    auto key = memory_->loadN(key_data, key_length);
    auto data = get(key, value_offset, value_length);
    if (not data) {
      return runtime::WasmMemory::kMaxMemorySize;
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
    auto put_result = db_->put(key, value);
    if (not put_result) {
      logger_->error(
          "ext_set_storage failed, due to fail in trie db with reason: {}",
          put_result.error().message());
    }
  }

  // -------------------------Trie operations--------------------------

  void StorageExtension::ext_blake2_256_enumerated_trie_root(
      runtime::WasmPointer values_data, runtime::WasmPointer lens_data,
      runtime::SizeType lens_length, runtime::WasmPointer result) {
    std::vector<common::Buffer> data;
    for (runtime::SizeType i = 0; i < lens_length; i++) {
      auto length = memory_->load32s(lens_data + i * 4);
      data.emplace_back(memory_->loadN(values_data, length));
      values_data += length;
    }

    auto root = codec_->orderedTrieRoot(data);
    memory_->storeBuffer(
        result, common::Buffer(std::vector<uint8_t>(root.begin(), root.end())));
  }

  runtime::SizeType StorageExtension::ext_storage_changes_root(
      runtime::WasmPointer parent_hash_data, runtime::SizeType parent_hash_len,
      runtime::SizeType parent_num, runtime::WasmPointer result) {
    // TODO (kamilsa): PRE-95 Implement ext_storage_changes_root, 03.04.2019
    logger_->error("Unimplemented");
    return 0;
  }

  void StorageExtension::ext_storage_root(runtime::WasmPointer result) const {
    const auto root = db_->getRoot();
    memory_->storeBuffer(result, root);
  }

  outcome::result<common::Buffer> StorageExtension::get(
      const common::Buffer &key, runtime::SizeType offset,
      runtime::SizeType max_length) const {
    OUTCOME_TRY(data, db_->get(key));

    const auto data_length =
        std::min<runtime::SizeType>(max_length, data.size() - offset);

    return common::Buffer(std::vector<uint8_t>(
        data.begin() + offset, data.begin() + offset + data_length));
  }

  // -------------------------Child storage--------------------------

  runtime::WasmPointer StorageExtension::ext_child_storage_root(
      runtime::WasmPointer storage_key_data,
      runtime::WasmPointer storage_key_length, runtime::WasmPointer written) {
    logger_->error("call to unimplemented ext_child_storage_root");
    std::terminate();
  }

  void StorageExtension::ext_clear_child_storage(
      runtime::WasmPointer storage_key_data,
      runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
      runtime::WasmPointer key_length) {
    logger_->error("call to unimplemented ext_clear_child_storage");
    std::terminate();
  }

  runtime::SizeType StorageExtension::ext_exists_child_storage(
      runtime::WasmPointer storage_key_data,
      runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
      runtime::WasmPointer key_length) {
    logger_->error("call to unimplemented ext_exists_child_storage");
    std::terminate();
  }

  runtime::WasmPointer StorageExtension::ext_get_allocated_child_storage(
      runtime::WasmPointer storage_key_data,
      runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
      runtime::WasmPointer key_length, runtime::WasmPointer written) {
    logger_->error("call to unimplemented ext_get_allocated_child_storage");
    std::terminate();
  }

  runtime::SizeType StorageExtension::ext_get_child_storage_into(
      runtime::WasmPointer storage_key_data,
      runtime::WasmPointer storage_key_length, runtime::WasmPointer key_data,
      runtime::WasmPointer key_length, runtime::WasmPointer value_data,
      runtime::SizeType value_length, runtime::SizeType value_offset) {
    logger_->error("call to unimplemented ext_get_child_storage_into");
    std::terminate();
  }

  void StorageExtension::ext_kill_child_storage(
      runtime::WasmPointer storage_key_data,
      runtime::SizeType storage_key_length) {
    logger_->error("call to unimplemented ext_kill_child_storage");
    std::terminate();
  }

  void StorageExtension::ext_set_child_storage(
      runtime::WasmPointer storage_key_data,
      runtime::SizeType storage_key_length, runtime::WasmPointer key_data,
      runtime::SizeType key_length, runtime::WasmPointer value_data,
      runtime::SizeType value_length) {
    logger_->error("call to unimplemented ext_set_child_storage");
    std::terminate();
  }

}  // namespace kagome::extensions
