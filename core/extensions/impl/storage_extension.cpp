/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/storage_extension.hpp"

#include <forward_list>

#include "primitives/block_id.hpp"
#include "runtime/wasm_result.hpp"
#include "storage/changes_trie/impl/changes_trie.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"

using kagome::common::Buffer;

namespace {
  const auto CHANGES_CONFIG_KEY = kagome::common::Buffer{}.put(":changes_trie");
}

namespace kagome::extensions {
  StorageExtension::StorageExtension(
      std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
      std::shared_ptr<runtime::WasmMemory> memory,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker)
      : storage_provider_(std::move(storage_provider)),
        memory_(std::move(memory)),
        changes_tracker_{std::move(changes_tracker)},
        logger_{common::createLogger(kDefaultLoggerTag)} {
    BOOST_ASSERT_MSG(storage_provider_ != nullptr, "storage batch is nullptr");
    BOOST_ASSERT_MSG(memory_ != nullptr, "memory is nullptr");
    BOOST_ASSERT_MSG(changes_tracker_ != nullptr, "changes tracker is nullptr");
  }

  // -------------------------Data storage--------------------------

  void StorageExtension::ext_clear_prefix(runtime::WasmPointer prefix_data,
                                          runtime::WasmSize prefix_length) {
    auto batch = storage_provider_->getCurrentBatch();
    auto prefix = memory_->loadN(prefix_data, prefix_length);
    auto res = batch->clearPrefix(prefix);
    if (not res) {
      logger_->error("ext_clear_prefix failed: {}", res.error().message());
    }
  }

  void StorageExtension::ext_clear_storage(runtime::WasmPointer key_data,
                                           runtime::WasmSize key_length) {
    auto batch = storage_provider_->getCurrentBatch();
    auto key = memory_->loadN(key_data, key_length);
    auto del_result = batch->remove(key);
    if (not del_result) {
      logger_->warn(
          "ext_clear_storage did not delete key {} from trie db with reason: "
          "{}",
          key_data,
          del_result.error().message());
    }
  }

  runtime::WasmSize StorageExtension::ext_exists_storage(
      runtime::WasmPointer key_data, runtime::WasmSize key_length) const {
    auto batch = storage_provider_->getCurrentBatch();
    auto key = memory_->loadN(key_data, key_length);
    return batch->contains(key) ? 1 : 0;
  }

  runtime::WasmPointer StorageExtension::ext_get_allocated_storage(
      runtime::WasmPointer key_data,
      runtime::WasmSize key_length,
      runtime::WasmPointer len_ptr) {
    auto batch = storage_provider_->getCurrentBatch();
    auto key = memory_->loadN(key_data, key_length);
    auto data = batch->get(key);
    const auto length = data.has_value() ? data.value().size()
                                         : runtime::WasmMemory::kMaxMemorySize;
    memory_->store32(len_ptr, length);

    if (not data) {
      return 0;
    }
    if (not data.value().empty())
      logger_->trace("ext_get_allocated_storage. Key hex: {} Value hex {}",
                     key.toHex(),
                     data.value().toHex());

    auto data_ptr = memory_->allocate(length);

    if (data_ptr != 0) {
      memory_->storeBuffer(data_ptr, data.value());
    } else {
      logger_->error(
          "ext_get_allocated_storage failed: memory could not allocate enough "
          "memory");
    }
    return data_ptr;
  }

  runtime::WasmSize StorageExtension::ext_get_storage_into(
      runtime::WasmPointer key_data,
      runtime::WasmSize key_length,
      runtime::WasmPointer value_data,
      runtime::WasmSize value_length,
      runtime::WasmSize value_offset) {
    auto key = memory_->loadN(key_data, key_length);
    auto data = get(key, value_offset, value_length);
    if (not data) {
      logger_->trace("ext_get_storage_into. Val by key {} not found",
                     key.toHex());
      return runtime::WasmMemory::kMaxMemorySize;
    }
    if (not data.value().empty()) {
      logger_->trace("ext_get_storage_into. Key hex: {} , Value hex {}",
                     key.toHex(),
                     data.value().toHex());
    } else {
      logger_->trace("ext_get_storage_into. Key hex: {} Value: empty",
                     key.toHex());
    }
    memory_->storeBuffer(value_data, data.value());
    return data.value().size();
  }

  runtime::WasmSpan StorageExtension::ext_storage_read_version_1(
      runtime::WasmSpan key_pos,
      runtime::WasmSpan value_out,
      runtime::WasmOffset offset) {
    static constexpr runtime::WasmSpan kErrorSpan = -1;

    auto [key_ptr, key_size] = runtime::WasmResult(key_pos);
    auto [value_ptr, value_size] = runtime::WasmResult(value_out);

    auto key = memory_->loadN(key_ptr, key_size);
    if (auto data = get(key, offset, value_size)) {
      memory_->storeBuffer(value_ptr, data.value());
      return runtime::WasmResult(0, data.value().size()).combine();
    }
    return kErrorSpan;
  }

  void StorageExtension::ext_set_storage(const runtime::WasmPointer key_data,
                                         runtime::WasmSize key_length,
                                         const runtime::WasmPointer value_data,
                                         runtime::WasmSize value_length) {
    auto key = memory_->loadN(key_data, key_length);
    auto value = memory_->loadN(value_data, value_length);

    if (value.toHex().size() < 500) {
      logger_->trace(
          "Set storage. Key: {}, Key hex: {} Value: {}, Value hex {}",
          key.toString(),
          key.toHex(),
          value.toString(),
          value.toHex());
    } else {
      logger_->trace(
          "Set storage. Key: {}, Key hex: {} Value is too big to display",
          key.toString(),
          key.toHex());
    }

    auto batch = storage_provider_->getCurrentBatch();
    auto put_result = batch->put(key, value);
    if (not put_result) {
      logger_->error(
          "ext_set_storage failed, due to fail in trie db with reason: {}",
          put_result.error().message());
    }
  }

  // -------------------------Trie operations--------------------------

  void StorageExtension::ext_blake2_256_enumerated_trie_root(
      runtime::WasmPointer values_data,
      runtime::WasmPointer lengths_data,
      runtime::WasmSize values_num,
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
    } else {
      logger_->error(
          "ext_blake2_256_enumerated_trie_root resulted with an error: {}",
          ordered_hash.error().message());
    }
  }

  runtime::WasmSize StorageExtension::ext_storage_changes_root(
      runtime::WasmPointer parent_hash_data, runtime::WasmPointer result) {
    if (not storage_provider_->isCurrentlyPersistent()) {
      logger_->error(
          "ext_storage_changes_root failed: called in ephemeral environment");
      return 0;
    }
    auto batch = storage_provider_->tryGetPersistentBatch().value();

    boost::optional<storage::changes_trie::ChangesTrieConfig> trie_config;
    auto config_bytes_res = batch->get(CHANGES_CONFIG_KEY);
    if (config_bytes_res.has_error()) {
      if (config_bytes_res.error() != storage::trie::TrieError::NO_VALUE) {
        logger_->error("ext_storage_changes_root resulted with an error: {}",
                       config_bytes_res.error().message());
        return 0;
      }
      logger_->debug("ext_storage_changes_root: no changes trie config found");
      trie_config = boost::none;
    } else {
      auto config_res = scale::decode<storage::changes_trie::ChangesTrieConfig>(
          config_bytes_res.value());
      if (config_res.has_error()) {
        logger_->error("ext_storage_changes_root resulted with an error: {}",
                       config_res.error().message());
        return 0;
      }
      trie_config = config_res.value();
    }

    auto parent_hash_bytes =
        memory_->loadN(parent_hash_data, common::Hash256::size());
    common::Hash256 parent_hash;
    std::copy_n(parent_hash_bytes.begin(),
                common::Hash256::size(),
                parent_hash.begin());

    // if no config found in the storage, then disable tracking blocks changes
    if (not trie_config.has_value()) {
      trie_config = storage::changes_trie::ChangesTrieConfig{
          .digest_interval = 0, .digest_levels = 0};
    }

    logger_->debug(
        "ext_storage_changes_root constructing changes trie with parent_hash: "
        "{}",
        parent_hash.toHex());
    auto trie_hash_res = changes_tracker_->constructChangesTrie(
        parent_hash, trie_config.value());
    if (trie_hash_res.has_error()) {
      logger_->error("ext_storage_changes_root resulted with an error: {}",
                     trie_hash_res.error().message());
      return 0;
    }
    common::Buffer result_buf(trie_hash_res.value());
    logger_->debug("ext_storage_changes_root with parent_hash {} result is: {}",
                   parent_hash.toHex(),
                   result_buf.toHex());
    memory_->storeBuffer(result, result_buf);
    return result_buf.size();
  }

  void StorageExtension::ext_storage_root(runtime::WasmPointer result) const {
    if (auto opt_batch = storage_provider_->tryGetPersistentBatch();
        opt_batch.has_value() and opt_batch.value() != nullptr) {
      auto res = opt_batch.value()->commit();
      if (res.has_error()) {
        logger_->error("ext_storage_root resulted with an error: {}",
                       res.error().message());
      }
      const auto &root = res.value();
      memory_->storeBuffer(result, root);
    } else {
      logger_->warn("ext_storage_root called in an ephemeral extension");
      auto res = storage_provider_->forceCommit();
      if (res.has_error()) {
        logger_->error("ext_storage_root resulted with an error: {}",
                       res.error().message());
      }
      const auto &root = res.value();
      memory_->storeBuffer(result, root);
    }
  }

  outcome::result<common::Buffer> StorageExtension::get(
      const common::Buffer &key,
      runtime::WasmSize offset,
      runtime::WasmSize max_length) const {
    auto batch = storage_provider_->getCurrentBatch();
    OUTCOME_TRY(data, batch->get(key));

    const auto data_length =
        std::min<runtime::WasmSize>(max_length, data.size() - offset);

    return common::Buffer(std::vector<uint8_t>(
        data.begin() + offset, data.begin() + offset + data_length));
  }

  outcome::result<common::Buffer> StorageExtension::get(
      const common::Buffer &key) const {
    auto batch = storage_provider_->getCurrentBatch();
    return batch->get(key);
  }

  outcome::result<boost::optional<Buffer>> StorageExtension::getStorageNextKey(
      const common::Buffer &key) const {
    auto batch = storage_provider_->getCurrentBatch();
    auto cursor = batch->cursor();
    OUTCOME_TRY(cursor->seek(key));
    OUTCOME_TRY(cursor->next());
    if (cursor->isValid()) {
      OUTCOME_TRY(next_key, cursor->key());
      return boost::make_optional(next_key);
    }
    return boost::none;
  }

  void StorageExtension::ext_storage_set_version_1(runtime::WasmSpan key,
                                                   runtime::WasmSpan value) {
    auto [key_ptr, key_size] = runtime::WasmResult(key);
    auto [value_ptr, value_size] = runtime::WasmResult(value);
    ext_set_storage(key_ptr, key_size, value_ptr, value_size);
  }

  runtime::WasmSpan StorageExtension::ext_storage_get_version_1(
      runtime::WasmSpan key) {
    auto [key_ptr, key_size] = runtime::WasmResult(key);
    auto key_buffer = memory_->loadN(key_ptr, key_size);
    auto data = get(key_buffer);
    if (not data) {
      logger_->trace("ext_get_storage_into. Val by key {} not found",
                     key_buffer.toHex());
      return runtime::WasmMemory::kMaxMemorySize;
    }
    if (not data.value().empty()) {
      logger_->trace("ext_get_storage_into. Key hex: {} , Value hex {}",
                     key_buffer.toHex(),
                     data.value().toHex());
    } else {
      logger_->trace("ext_get_storage_into. Key hex: {} Value: empty",
                     key_buffer.toHex());
    }
    return memory_->storeBuffer(data.value());
  }

  void StorageExtension::ext_storage_clear_version_1(
      runtime::WasmSpan key_data) {
    auto [key_ptr, key_size] = runtime::WasmResult(key_data);
    ext_clear_storage(key_ptr, key_size);
  }

  runtime::WasmSize StorageExtension::ext_storage_exists_version_1(
      runtime::WasmSpan key_data) const {
    auto [key_ptr, key_size] = runtime::WasmResult(key_data);
    return ext_exists_storage(key_ptr, key_size);
  }

  void StorageExtension::ext_storage_clear_prefix_version_1(
      runtime::WasmSpan prefix) {
    auto [prefix_ptr, prefix_size] = runtime::WasmResult(prefix);
    return ext_clear_prefix(prefix_ptr, prefix_size);
  }

  runtime::WasmPointer StorageExtension::ext_storage_root_version_1() const {
    auto hash_size = common::Hash256::size();
    auto ptr = memory_->allocate(hash_size);
    ext_storage_root(ptr);
    return ptr;
  }

  runtime::WasmPointer StorageExtension::ext_storage_changes_root_version_1(
      runtime::WasmSpan parent_hash_data) {
    auto hash_size = common::Hash256::size();
    auto result = memory_->allocate(hash_size);
    auto parent_hash_ptr = runtime::WasmResult(parent_hash_data).address;
    auto bytes_written = ext_storage_changes_root(parent_hash_ptr, result);
    if (hash_size != bytes_written) {
      std::terminate();
    }

    return result;
  }

  runtime::WasmSpan StorageExtension::ext_storage_next_key_version_1(
      runtime::WasmSpan key_span) const {
    static constexpr runtime::WasmSpan kErrorSpan = -1;

    auto [key_ptr, key_size] = runtime::WasmResult(key_span);
    auto key_bytes = memory_->loadN(key_ptr, key_size);
    auto res = getStorageNextKey(key_bytes);
    if (res.has_error()) {
      logger_->error("ext_storage_next_key resulted with error: {}",
                     res.error().message());
      return kErrorSpan;
    }
    auto &&key_opt = res.value();
    if (auto enc_res = scale::encode(key_opt); enc_res.has_value()) {
      return memory_->storeBuffer(enc_res.value());
    } else {  // NOLINT(readability-else-after-return)
      logger_->error(
          "ext_storage_next_key result encoding resulted with error: {}",
          enc_res.error().message());
    }
    return kErrorSpan;
  }

  namespace {
    /**
     * @brief type of serialized data for ext_trie_blake2_256_root_version_1
     */
    using KeyValueCollection =
        std::vector<std::pair<common::Buffer, common::Buffer>>;
    /**
     * @brief type of serialized data for
     * ext_trie_blake2_256_ordered_root_version_1
     */
    using ValuesCollection = std::vector<common::Buffer>;
  }  // namespace

  runtime::WasmPointer StorageExtension::ext_trie_blake2_256_root_version_1(
      runtime::WasmSpan values_data) {
    auto [ptr, size] = runtime::WasmResult(values_data);
    const auto &buffer = memory_->loadN(ptr, size);
    const auto &pairs = scale::decode<KeyValueCollection>(buffer);
    if (!pairs) {
      logger_->error("failed to decode pairs: {}", pairs.error().message());
      std::terminate();
    }

    auto &&pv = pairs.value();
    storage::trie::PolkadotCodec codec;
    if (pv.empty()) {
      static const auto empty_root = common::Buffer{}.put(codec.hash256({0}));
      auto res = memory_->storeBuffer(empty_root);
      return runtime::WasmResult(res).address;
    }
    storage::trie::PolkadotTrieImpl trie;
    for (auto &&p : pv) {
      auto &&key = p.first;
      auto &&value = p.second;
      // already scale-encoded
      trie.put(key, value);
    }
    const auto &enc = codec.encodeNode(*trie.getRoot());
    if (!enc) {
      logger_->error("failed to encode trie root: {}", enc.error().message());
      std::terminate();
    }
    const auto &hash = codec.hash256(enc.value());

    std::cout << "res hash = " << hash.toHex() << std::endl;

    auto res = memory_->storeBuffer(hash);
    return runtime::WasmResult(res).address;
  }

  runtime::WasmPointer
  StorageExtension::ext_trie_blake2_256_ordered_root_version_1(
      runtime::WasmSpan values_data) {
    auto [ptr, size] = runtime::WasmResult(values_data);
    const auto &buffer = memory_->loadN(ptr, size);
    const auto &values = scale::decode<ValuesCollection>(buffer);
    if (!values) {
      logger_->error("failed to decode values: {}", values.error().message());
      std::terminate();
    }

    const auto &collection = values.value();
    auto ordered_hash = storage::trie::calculateOrderedTrieHash(
        collection.begin(), collection.end());
    if (!ordered_hash.has_value()) {
      logger_->error(
          "ext_blake2_256_enumerated_trie_root resulted with an error: {}",
          ordered_hash.error().message());
      std::terminate();
    }

    auto res = memory_->storeBuffer(ordered_hash.value());
    return runtime::WasmResult(res).address;
  }

}  // namespace kagome::extensions
