/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/storage_extension.hpp"

#include <algorithm>
#include <thread>

#include <fmt/std.h>

#include "clock/impl/clock_impl.hpp"
#include "common/monadic_utils.hpp"
#include "common/outcome_throw.hpp"
#include "crypto/blake2/blake2b.h"
#include "crypto/hasher.hpp"
#include "crypto/keccak/keccak.hpp"
#include "host_api/impl/storage_util.hpp"
#include "log/trace_macros.hpp"
#include "runtime/common/runtime_execution_error.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/ptr_size.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "scale/encode_append.hpp"
#include "scale/kagome_scale.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/impl/topper_trie_batch_impl.hpp"
#include "storage/trie/serialization/ordered_trie_hash.hpp"

using kagome::common::Buffer;

namespace kagome::host_api {
  StorageExtension::StorageExtension(
      std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
      std::shared_ptr<const runtime::MemoryProvider> memory_provider,
      std::shared_ptr<const crypto::Hasher> hasher,
      LazySPtr<api::StateApi> state_api)
      : storage_provider_(std::move(storage_provider)),
        memory_provider_(std::move(memory_provider)),
        hasher_(std::move(hasher)),
        state_api_(std::move(state_api)),
        logger_{log::createLogger("StorageExtension", "storage_extension")} {
    BOOST_ASSERT(storage_provider_ != nullptr);
    BOOST_ASSERT(memory_provider_ != nullptr);
    BOOST_ASSERT(hasher_ != nullptr);
  }

  void StorageExtension::reset() {
    // rollback will have value until there are opened transactions that need
    // to be closed
    for (; transactions_ != 0; --transactions_) {
      if (auto res = storage_provider_->rollbackTransaction();
          res.has_error()) {
        if (res.error()
            != runtime::RuntimeExecutionError::NO_TRANSACTIONS_WERE_STARTED) {
          logger_->error(res.error());
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
        common::BufferView data = *data_opt;
        data = data.subspan(std::min<size_t>(offset, data.size()));
        auto written = std::min<size_t>(data.size(), value.size);
        memory.storeBuffer(value.ptr, data.first(written));
        res = data.size();

        SL_TRACE_FUNC_CALL(
            logger_, data, key, common::Buffer{data.first(written)});
      } else {
        SL_TRACE_FUNC_CALL(
            logger_, std::string_view{"none"}, key, value_out, offset);
      }
    } else {
      SL_ERROR(logger_,
               "Error in ext_storage_read_version_1: {}",
               data_opt_res.error());
    }
    return memory.storeBuffer(scale::encode(res).value());
  }

  outcome::result<std::optional<common::BufferOrView>> StorageExtension::get(
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

    // Special handling for runtime code updates based on system version
    if (key == storage::kRuntimeCodeKey) {
      auto state_api = state_api_.get();
      if (state_api) {
        uint8_t system_version = 0;

        const auto runtime_version_res =
            state_api->getRuntimeVersion(std::nullopt);
        if (runtime_version_res.has_value()) {
          system_version = runtime_version_res.value().system_version;
        }

        if (system_version >= 3) {
          SL_INFO(logger_,
                  "Storing runtime code in :pending_code (system_version: {})",
                  system_version);
          auto put_result = batch->put(storage::kPendingRuntimeCodeKey, value);
          if (not put_result) {
            logger_->error(
                "ext_set_storage failed to store pending code, reason: {}",
                put_result.error());
          }
          return;
        }
      }
    }

    auto put_result = batch->put(key, value);
    if (not put_result) {
      logger_->error(
          "ext_set_storage failed, due to fail in trie db with reason: {}",
          put_result.error());
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
      auto msg = fmt::format(error_message, key_buffer.toHex(), result.error());
      SL_DEBUG(logger_, "{}", msg);
      throw std::runtime_error(msg);
    }

    auto &option = result.value();

    return memory.storeBuffer(
        scale::encode(common::map_optional(option, [](auto &r) {
          return r.view();
        })).value());
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
          del_result.error());
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

  void StorageExtension::ext_storage_clear_prefix_version_1(BufferView prefix) {
    storage_provider_->clearPrefix(std::nullopt, prefix, std::nullopt);
  }

  KillStorageResult StorageExtension::ext_storage_clear_prefix_version_2(
      BufferView prefix, ClearPrefixLimit limit) {
    return storage_provider_->clearPrefix(std::nullopt, prefix, limit);
  }

  Hash256 StorageExtension::ext_storage_root_version_1() {
    return ext_storage_root_version_2(storage::trie::StateVersion::V0);
  }

  Hash256 StorageExtension::ext_storage_root_version_2(
      storage::trie::StateVersion version) {
    return storage_provider_->commit(std::nullopt, version).value();
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
                     res.error());
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
          enc_res.error());
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
      throw std::runtime_error(fmt::format(
          "Error fetching value from storage: {}", val_opt_res.error()));
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
            put_result.error());
      }
      return;
    }
  }

  void StorageExtension::ext_storage_start_transaction_version_1() {
    auto res = storage_provider_->startTransaction();
    SL_TRACE_VOID_FUNC_CALL(logger_);
    if (res.has_error()) {
      logger_->error("Storage transaction start has failed: {}", res.error());
      res.value();
    }
    ++transactions_;
  }

  void StorageExtension::ext_storage_commit_transaction_version_1() {
    auto res = storage_provider_->commitTransaction();
    SL_TRACE_VOID_FUNC_CALL(logger_);
    if (res.has_error()) {
      logger_->error("Storage transaction rollback has failed: {}",
                     res.error());
      res.value();
    }
    --transactions_;
  }

  void StorageExtension::ext_storage_rollback_transaction_version_1() {
    auto res = storage_provider_->rollbackTransaction();
    SL_TRACE_VOID_FUNC_CALL(logger_);
    if (res.has_error()) {
      logger_->error("Storage transaction rollback has failed: {}",
                     res.error());
      res.value();
    }
    --transactions_;
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
      logger_->error("failed to decode pairs: {}", pairs.error());
    }

    auto &&pv = pairs.value();
    storage::trie::PolkadotCodec codec{crypto::blake2b<32>};
    if (pv.empty()) {
      auto res = memory.storeBuffer(storage::trie::kEmptyRootHash);
      return runtime::PtrSize(res).ptr;
    }
    auto trie = storage::trie::PolkadotTrieImpl::createEmpty();
    for (auto &&p : pv) {
      auto &&key = p.first;
      common::BufferView value = p.second;
      // already scale-encoded
      auto put_res = trie->put(key, value);
      if (not put_res) {
        logger_->error(
            "Insertion of value {} with key {} into the trie failed due to "
            "error: {}",
            value.toHex(),
            key.toHex(),
            put_res.error());
      }
    }
    const auto &enc =
        codec.encodeNode(*trie->getRoot(), storage::trie::StateVersion::V0, {});
    if (!enc) {
      logger_->error("failed to encode trie root: {}", enc.error());
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
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [address, size] = runtime::PtrSize(values_data);
    auto values_res =
        scale::decode<ValuesCollection>(memory.loadN(address, size));
    common::raise_on_err(values_res);

    const auto &values = values_res.value();

    auto ordered_hash =
        storage::trie::calculateOrderedTrieHash(detail::toStateVersion(version),
                                                values.begin(),
                                                values.end(),
                                                crypto::blake2b<32>);
    common::raise_on_err(ordered_hash);

    SL_TRACE_FUNC_CALL(logger_, ordered_hash.value());
    return runtime::PtrSize(memory.storeBuffer(ordered_hash.value())).ptr;
  }

  runtime::WasmPointer
  StorageExtension::ext_trie_keccak_256_ordered_root_version_2(
      runtime::WasmSpan values_data, runtime::WasmI32 version) {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [address, size] = runtime::PtrSize(values_data);
    auto values_res =
        scale::decode<ValuesCollection>(memory.loadN(address, size));
    common::raise_on_err(values_res);

    const auto &values = values_res.value();

    auto ordered_hash =
        storage::trie::calculateOrderedTrieHash(detail::toStateVersion(version),
                                                values.begin(),
                                                values.end(),
                                                crypto::keccak);
    common::raise_on_err(ordered_hash);

    SL_TRACE_FUNC_CALL(logger_, ordered_hash.value());
    return runtime::PtrSize(memory.storeBuffer(ordered_hash.value())).ptr;
  }
}  // namespace kagome::host_api
