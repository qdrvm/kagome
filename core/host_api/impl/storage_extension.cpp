/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/storage_extension.hpp"

#include <algorithm>

#include "clock/impl/clock_impl.hpp"
#include "log/profiling_logger.hpp"
#include "runtime/common/runtime_transaction_error.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/ptr_size.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "scale/encode_append.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/impl/topper_trie_batch_impl.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"

using kagome::common::Buffer;

namespace {
  [[nodiscard]] kagome::storage::trie::StateVersion toStateVersion(
      kagome::runtime::WasmI32 state_version_int) {
    if (state_version_int == 0) {
      return kagome::storage::trie::StateVersion::V0;
    } else if (state_version_int == 1) {
      // TODO(xDimon): remove exception when new version will be implemented
      throw std::runtime_error("StateVersion::V1 is not implemented");
      return kagome::storage::trie::StateVersion::V1;
    } else {
      throw std::runtime_error(fmt::format(
          "Invalid state version: {}. Expected 0 or 1", state_version_int));
    }
  }
}  // namespace

namespace kagome::host_api {
  StorageExtension::StorageExtension(
      std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
      std::shared_ptr<const runtime::MemoryProvider> memory_provider)
      : storage_provider_(std::move(storage_provider)),
        memory_provider_(std::move(memory_provider)),
        logger_{log::createLogger("StorageExtension", "storage_extension")} {
    BOOST_ASSERT_MSG(storage_provider_ != nullptr, "storage batch is nullptr");
    BOOST_ASSERT_MSG(memory_provider_ != nullptr, "memory provider is nullptr");
  }

  void StorageExtension::reset() {
    // rollback will have value until there are opened transactions that need
    // to be closed
    while (true) {
      if (auto res = storage_provider_->rollbackTransaction();
          res.has_error()) {
        if (res.error()
            != runtime::RuntimeTransactionError::NO_TRANSACTIONS_WERE_STARTED) {
          logger_->error(res.error().message());
        }
        break;
      }
    }
  }

  // -------------------------Data storage--------------------------

  runtime::WasmSpan StorageExtension::ext_storage_read_version_1(
      runtime::WasmSpan key_pos,
      runtime::WasmSpan value_out,
      runtime::WasmOffset offset) {
    auto [key_ptr, key_size] = runtime::PtrSize(key_pos);
    auto value = runtime::PtrSize(value_out);
    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto key = memory.loadN(key_ptr, key_size);
    std::optional<uint32_t> res{std::nullopt};
    if (auto data_opt_res = get(key); data_opt_res.has_value()) {
      auto &data_opt = data_opt_res.value();
      if (data_opt.has_value()) {
        common::BufferView data = data_opt.value().get();
        data = data.subspan(std::min<size_t>(offset, data.size()));
        auto written = std::min<size_t>(data.size(), value.size);
        memory.storeBuffer(value.ptr, data.subspan(0, written));
        res = data.size();

        SL_TRACE_FUNC_CALL(
            logger_, data, key, common::Buffer{data.subspan(0, written)});
      } else {
        SL_TRACE_FUNC_CALL(
            logger_, std::string_view{"none"}, key, value_out, offset);
      }
    } else {
      SL_ERROR(logger_,
               "Error in ext_storage_read_version_1: {}",
               data_opt_res.error().message());
    }
    return memory.storeBuffer(scale::encode(res).value());
  }

  outcome::result<std::optional<common::BufferConstRef>> StorageExtension::get(
      const common::BufferView &key) const {
    auto batch = storage_provider_->getCurrentBatch();
    return batch->tryGet(key);
  }

  common::Buffer StorageExtension::loadKey(runtime::WasmSpan key) const {
    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto &memory = memory_provider_->getCurrentMemory()->get();
    return memory.loadN(key_ptr, key_size);
  }

  outcome::result<std::optional<Buffer>> StorageExtension::getStorageNextKey(
      const common::Buffer &key) const {
    auto batch = storage_provider_->getCurrentBatch();
    auto cursor = batch->trieCursor();
    OUTCOME_TRY(cursor->seekUpperBound(key));
    return cursor->key();
  }

  void StorageExtension::ext_storage_set_version_1(
      runtime::WasmSpan key_span, runtime::WasmSpan value_span) {
    auto [key_ptr, key_size] = runtime::PtrSize(key_span);
    auto [value_ptr, value_size] = runtime::PtrSize(value_span);
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto key = memory.loadN(key_ptr, key_size);
    auto value = memory.loadN(value_ptr, value_size);

    SL_TRACE_VOID_FUNC_CALL(logger_, key, value);

    auto batch = storage_provider_->getCurrentBatch();
    auto put_result = batch->put(key, value);
    if (not put_result) {
      logger_->error(
          "ext_set_storage failed, due to fail in trie db with reason: {}",
          put_result.error().message());
    }
  }

  runtime::WasmSpan StorageExtension::ext_storage_get_version_1(
      runtime::WasmSpan key) {
    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto key_buffer = memory.loadN(key_ptr, key_size);

    constexpr auto error_message =
        "ext_storage_get_version_1( {} ) => value was not obtained. Reason: {}";

    auto result = get(key_buffer);

    if (result) {
      SL_TRACE_FUNC_CALL(logger_, result.value(), key_buffer);
    } else {
      logger_->error(
          error_message, key_buffer.toHex(), result.error().message());
    }

    auto &option = result.value();

    return memory.storeBuffer(scale::encode(option).value());
  }

  void StorageExtension::ext_storage_clear_version_1(
      runtime::WasmSpan key_data) {
    auto [key_ptr, key_size] = runtime::PtrSize(key_data);
    auto batch = storage_provider_->getCurrentBatch();
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto key = memory.loadN(key_ptr, key_size);
    auto del_result = batch->remove(key);
    SL_TRACE_FUNC_CALL(logger_, del_result.has_value(), key);
    if (not del_result) {
      logger_->warn(
          "ext_storage_clear_version_1 did not delete key {} from trie db "
          "with reason: {}",
          key_data,
          del_result.error().message());
    }
  }

  runtime::WasmSize StorageExtension::ext_storage_exists_version_1(
      runtime::WasmSpan key_data) const {
    auto [key_ptr, key_size] = runtime::PtrSize(key_data);
    auto batch = storage_provider_->getCurrentBatch();
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto key = memory.loadN(key_ptr, key_size);
    auto res = batch->contains(key);
    return (res.has_value() and res.value()) ? 1 : 0;
  }

  void StorageExtension::ext_storage_clear_prefix_version_1(
      runtime::WasmSpan prefix_span) {
    auto [prefix_ptr, prefix_size] = runtime::PtrSize(prefix_span);
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto prefix = memory.loadN(prefix_ptr, prefix_size);
    SL_TRACE_VOID_FUNC_CALL(logger_, prefix);
    (void)clearPrefix(prefix, std::nullopt);
  }

  runtime::WasmSpan StorageExtension::ext_storage_clear_prefix_version_2(
      runtime::WasmSpan prefix_span, runtime::WasmSpan limit_span) {
    auto [prefix_ptr, prefix_size] = runtime::PtrSize(prefix_span);
    auto [limit_ptr, limit_size] = runtime::PtrSize(limit_span);
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto prefix = memory.loadN(prefix_ptr, prefix_size);
    auto enc_limit = memory.loadN(limit_ptr, limit_size);
    auto limit_res = scale::decode<std::optional<uint32_t>>(enc_limit);
    if (!limit_res) {
      auto msg = fmt::format(
          "ext_storage_clear_prefix_version_2 failed at decoding second "
          "argument: {}",
          limit_res.error());
      logger_->error(msg);
      throw std::runtime_error(msg);
    }
    auto limit_opt = std::move(limit_res.value());
    if (limit_opt) {
      SL_TRACE_VOID_FUNC_CALL(logger_, prefix, limit_opt.value());
    } else {
      SL_TRACE_VOID_FUNC_CALL(logger_, prefix, std::string_view{"none"});
    }
    return clearPrefix(prefix, limit_opt);
  }

  runtime::WasmSpan StorageExtension::ext_storage_root_version_1() {
    return ext_storage_root_version_2(runtime::WasmI32(0));
  }

  runtime::WasmSpan StorageExtension::ext_storage_root_version_2(
      runtime::WasmI32 version) {
    [[maybe_unused]] auto state_version = toStateVersion(version);

    outcome::result<storage::trie::RootHash> res{{}};
    removeEmptyChildStorages();
    if (auto opt_batch = storage_provider_->tryGetPersistentBatch();
        opt_batch.has_value() and opt_batch.value() != nullptr) {
      res = opt_batch.value()->commit();
    } else {
      logger_->warn("ext_storage_root called in an ephemeral extension");
      res = storage_provider_->forceCommit();
    }
    if (res.has_error()) {
      logger_->error("ext_storage_root resulted with an error: {}",
                     res.error().message());
    }
    const auto &root = res.value();
    auto &memory = memory_provider_->getCurrentMemory()->get();
    return memory.storeBuffer(root);
  }

  runtime::WasmSpan StorageExtension::ext_storage_changes_root_version_1(
      runtime::WasmSpan parent_hash_data) {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    // https://github.com/paritytech/substrate/pull/10080
    return memory.storeBuffer(scale::encode(std::optional<Buffer>()).value());
  }

  runtime::WasmSpan StorageExtension::ext_storage_next_key_version_1(
      runtime::WasmSpan key_span) const {
    static constexpr runtime::WasmSpan kErrorSpan = -1;

    auto [key_ptr, key_size] = runtime::PtrSize(key_span);
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto key_bytes = memory.loadN(key_ptr, key_size);
    auto res = getStorageNextKey(key_bytes);
    if (res.has_error()) {
      logger_->error("ext_storage_next_key resulted with error: {}",
                     res.error().message());
      return kErrorSpan;
    }
    auto &&next_key_opt = res.value();
    if (auto enc_res = scale::encode(next_key_opt); enc_res.has_value()) {
      SL_TRACE_FUNC_CALL(logger_,
                         res.value().has_value()
                             ? res.value().value()
                             : common::Buffer().put("no value"),
                         key_bytes);
      return memory.storeBuffer(enc_res.value());
    } else {  // NOLINT(readability-else-after-return)
      logger_->error(
          "ext_storage_next_key result encoding resulted with error: {}",
          enc_res.error().message());
    }
    return kErrorSpan;
  }

  void StorageExtension::ext_storage_append_version_1(
      runtime::WasmSpan key_span, runtime::WasmSpan append_span) const {
    auto [key_ptr, key_size] = runtime::PtrSize(key_span);
    auto [append_ptr, append_size] = runtime::PtrSize(append_span);
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto key_bytes = memory.loadN(key_ptr, key_size);
    auto append_bytes = memory.loadN(append_ptr, append_size);

    auto val_opt_res = get(key_bytes);
    if (val_opt_res.has_error()) {
      throw std::runtime_error{
          fmt::format("Error fetching value from storage: {}",
                      val_opt_res.error().message())};
    }
    auto &val_opt = val_opt_res.value();
    auto &&val = val_opt ? common::Buffer{val_opt.value()} : common::Buffer{};

    if (scale::append_or_new_vec(val.asVector(), append_bytes).has_value()) {
      auto batch = storage_provider_->getCurrentBatch();
      SL_TRACE_VOID_FUNC_CALL(logger_, key_bytes, val);
      auto put_result = batch->put(key_bytes, std::move(val));
      if (not put_result) {
        logger_->error(
            "ext_storage_append_version_1 failed, due to fail in trie db "
            "with reason: {}",
            put_result.error().message());
      }
      return;
    }
  }

  void StorageExtension::ext_storage_start_transaction_version_1() {
    auto res = storage_provider_->startTransaction();
    if (res.has_error()) {
      logger_->error("Storage transaction start has failed: {}",
                     res.error().message());
      throw std::runtime_error(res.error().message());
    }
  }

  void StorageExtension::ext_storage_commit_transaction_version_1() {
    auto res = storage_provider_->commitTransaction();
    SL_TRACE_VOID_FUNC_CALL(logger_);
    if (res.has_error()) {
      logger_->error("Storage transaction rollback has failed: {}",
                     res.error().message());
      throw std::runtime_error(res.error().message());
    }
  }

  void StorageExtension::ext_storage_rollback_transaction_version_1() {
    auto res = storage_provider_->rollbackTransaction();
    SL_TRACE_VOID_FUNC_CALL(logger_);
    if (res.has_error()) {
      logger_->error("Storage transaction commit has failed: {}",
                     res.error().message());
      throw std::runtime_error(res.error().message());
    }
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
    auto [ptr, size] = runtime::PtrSize(values_data);
    auto &memory = memory_provider_->getCurrentMemory()->get();
    const auto &buffer = memory.loadN(ptr, size);
    const auto &pairs = scale::decode<KeyValueCollection>(buffer);
    if (!pairs) {
      logger_->error("failed to decode pairs: {}", pairs.error().message());
      throw std::runtime_error(pairs.error().message());
    }

    auto &&pv = pairs.value();
    storage::trie::PolkadotCodec codec;
    if (pv.empty()) {
      static const auto empty_root =
          common::Buffer(codec.hash256(common::Buffer{0}));
      auto res = memory.storeBuffer(empty_root);
      return runtime::PtrSize(res).ptr;
    }
    storage::trie::PolkadotTrieImpl trie;
    for (auto &&p : pv) {
      auto &&key = p.first;
      auto &&value = p.second;
      // already scale-encoded
      auto put_res = trie.put(key, value);
      if (not put_res) {
        logger_->error(
            "Insertion of value {} with key {} into the trie failed due to "
            "error: {}",
            value.toHex(),
            key.toHex(),
            put_res.error().message());
      }
    }
    const auto &enc = codec.encodeNode(*trie.getRoot());
    if (!enc) {
      logger_->error("failed to encode trie root: {}", enc.error().message());
      throw std::runtime_error(enc.error().message());
    }
    const auto &hash = codec.hash256(enc.value());

    auto res = memory.storeBuffer(hash);
    return runtime::PtrSize(res).ptr;
  }

  runtime::WasmPointer
  StorageExtension::ext_trie_blake2_256_ordered_root_version_1(
      runtime::WasmSpan values_data) {
    return ext_trie_blake2_256_ordered_root_version_2(values_data,
                                                      runtime::WasmI32(0));
  }

  runtime::WasmPointer
  StorageExtension::ext_trie_blake2_256_ordered_root_version_2(
      runtime::WasmSpan values_data, runtime::WasmI32 version) {
    auto [address, size] = runtime::PtrSize(values_data);
    auto &memory = memory_provider_->getCurrentMemory()->get();
    const auto &buffer = memory.loadN(address, size);
    const auto &values = scale::decode<ValuesCollection>(buffer);
    if (!values) {
      logger_->error("failed to decode values: {}", values.error().message());
      throw std::runtime_error(values.error().message());
    }
    const auto &collection = values.value();

    [[maybe_unused]] auto state_version = toStateVersion(version);

    auto ordered_hash = storage::trie::calculateOrderedTrieHash(
        collection.begin(), collection.end());
    if (!ordered_hash.has_value()) {
      logger_->error(
          "ext_blake2_256_enumerated_trie_root resulted with an error: {}",
          ordered_hash.error().message());
      throw std::runtime_error(ordered_hash.error().message());
    }
    SL_TRACE_FUNC_CALL(logger_, ordered_hash.value());
    auto res = memory.storeBuffer(ordered_hash.value());
    return runtime::PtrSize(res).ptr;
  }

  runtime::WasmSpan StorageExtension::clearPrefix(
      common::BufferView prefix, std::optional<uint32_t> limit) {
    auto batch = storage_provider_->getCurrentBatch();
    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto res = batch->clearPrefix(
        prefix, limit ? std::optional<uint64_t>(limit.value()) : std::nullopt);
    if (not res) {
      auto msg = fmt::format("ext_storage_clear_prefix failed: {}",
                             res.error().message());
      logger_->error(msg);
      throw std::runtime_error(msg);
    }
    auto enc_res = scale::encode(res.value());
    if (not enc_res) {
      auto msg = fmt::format("ext_storage_clear_prefix failed: {}",
                             enc_res.error().message());
      logger_->error(msg);
      throw std::runtime_error(msg);
    }
    return memory.storeBuffer(enc_res.value());
  }

  void StorageExtension::removeEmptyChildStorages() {
    static const auto &prefix = storage::kChildStorageDefaultPrefix;
    static const auto empty_hash = codec_.hash256(common::Buffer{0});
    auto current_key = prefix;
    auto key_res = getStorageNextKey(current_key);
    while (key_res.has_value() and key_res.value().has_value()) {
      auto &key_opt = key_res.value();
      current_key = key_opt.value();

      bool contains_prefix =
          std::equal(prefix.begin(), prefix.end(), current_key.begin());
      if (not contains_prefix) {
        break;
      }
      // SAFETY: key obtained by getStorageNextKey method, thus must exist in
      // the storage
      auto value_opt = get(current_key).value();
      if (value_opt and value_opt.value().get() == empty_hash) {
        auto batch = storage_provider_->getCurrentBatch();
        auto remove_res = batch->remove(current_key);
        if (not remove_res) {
          logger_->error(
              "Unable to remove empty child storage under key {}, error is {}",
              current_key,
              remove_res.error().message());
        } else {
          SL_TRACE(
              logger_, "Removed empty child trie under key {}", current_key);
        }
      }
      key_res = getStorageNextKey(current_key);
    }
  }

}  // namespace kagome::host_api
