/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/child_storage_extension.hpp"

#include "runtime/common/runtime_transaction_error.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/ptr_size.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "scale/encode_append.hpp"

using kagome::common::Buffer;

namespace kagome::host_api {

  ChildStorageExtension::ChildStorageExtension(
      std::shared_ptr<runtime::TrieStorageProvider> storage_provider,
      std::shared_ptr<const runtime::MemoryProvider> memory_provider)
      : storage_provider_(std::move(storage_provider)),
        memory_provider_(std::move(memory_provider)),
        logger_{log::createLogger("ChildStorageExtension",
                                  "child_storage_extension")} {
    BOOST_ASSERT_MSG(storage_provider_ != nullptr, "storage batch is nullptr");
    BOOST_ASSERT_MSG(memory_provider_ != nullptr, "memory provider is nullptr");
  }

  template <typename F>
  auto ChildStorageExtension::executeOnChildStorage(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan key, F func) {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [child_storage_key_ptr, child_storage_key_size] =
        runtime::PtrSize(child_storage_key);
    auto child_key_buffer =
        memory.loadN(child_storage_key_ptr, child_storage_key_size);
    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);

    auto child_batch = storage_provider_->getChildBatchAt(child_key_buffer);
    return func(child_batch);
  }

  common::Buffer make_prefixed_child_storage_key(
      const common::Buffer &child_storage_key) {
    return common::Buffer::fromString(":child_storage:default:")
        .value()
        .putBuffer(child_storage_key);
  }

  void ChildStorageExtension::ext_default_child_storage_set_version_1(
      runtime::WasmSpan child_storage_key,
      runtime::WasmSpan key,
      runtime::WasmSpan value) {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [child_storage_key_ptr, child_storage_key_size] =
        runtime::PtrSize(child_storage_key);
    auto child_key_buffer =
        memory.loadN(child_storage_key_ptr, child_storage_key_size);
    auto prefixed_child_key = make_prefixed_child_storage_key(child_key_buffer);
    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);

    auto [value_ptr, value_size] = runtime::PtrSize(value);
    auto value_buffer = memory.loadN(value_ptr, value_size);

    SL_TRACE_VOID_FUNC_CALL(
        logger_, child_key_buffer, key_buffer, value_buffer);

    auto inner_batch = storage_provider_->getChildBatchAt(prefixed_child_key);
    auto put_result = inner_batch.value()->put(key_buffer, value_buffer);

    auto new_child_root = inner_batch.value()->commit().value();
    storage_provider_->getCurrentBatch()->put(
        prefixed_child_key,
        common::Buffer{scale::encode(new_child_root).value()});
    storage_provider_->getChildBatches().clear();

    if (not put_result) {
      logger_->error(
          "ext_default_child_storage_set_version_1 failed, due to fail in trie "
          "db with reason: {}",
          put_result.error().message());
    }
  }

  runtime::WasmSpan
  ChildStorageExtension::ext_default_child_storage_get_version_1(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan key) const {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [child_storage_key_ptr, child_storage_key_size] =
        runtime::PtrSize(child_storage_key);
    auto child_key_buffer =
        memory.loadN(child_storage_key_ptr, child_storage_key_size);
    auto prefixed_child_key = make_prefixed_child_storage_key(child_key_buffer);

    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);

    SL_TRACE_VOID_FUNC_CALL(logger_, child_key_buffer, key_buffer);

    auto inner_batch = storage_provider_->getChildBatchAt(prefixed_child_key);
    auto result = inner_batch.value()->get(key_buffer);
    auto option = result ? std::make_optional(result.value()) : std::nullopt;

    if (option) {
      SL_TRACE_FUNC_CALL(logger_, option.value(), child_key_buffer, key_buffer);
    } else {
      SL_TRACE(logger_,
               "ext_default_child_storage_get_version_1( {}, {} ) => value was "
               "not obtained. "
               "Reason: {}",
               child_key_buffer.toHex(),
               key_buffer.toHex(),
               result.error().message());
    }
    return memory.storeBuffer(scale::encode(option).value());
  }

  void ChildStorageExtension::ext_default_child_storage_clear_version_1(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan key) {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [child_storage_key_ptr, child_storage_key_size] =
        runtime::PtrSize(child_storage_key);
    auto child_key_buffer =
        memory.loadN(child_storage_key_ptr, child_storage_key_size);
    auto prefixed_child_key = make_prefixed_child_storage_key(child_key_buffer);

    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);

    SL_TRACE_VOID_FUNC_CALL(logger_, child_key_buffer, key_buffer);

    auto inner_batch = storage_provider_->getChildBatchAt(prefixed_child_key);
    auto result = inner_batch.value()->remove(key_buffer);
    auto new_child_root = inner_batch.value()->commit().value();
    storage_provider_->getCurrentBatch()->put(
        prefixed_child_key,
        common::Buffer{scale::encode(new_child_root).value()});
    storage_provider_->getChildBatches().clear();
    if (not result) {
      logger_->error(
          "ext_default_child_storage_clear_version_1 failed, due to fail in "
          "trie "
          "db with reason: {}",
          result.error().message());
    }
  }

  runtime::WasmSpan
  ChildStorageExtension::ext_default_child_storage_next_key_version_1(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan key) const {
    static constexpr runtime::WasmSpan kErrorSpan = -1;
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [child_storage_key_ptr, child_storage_key_size] =
        runtime::PtrSize(child_storage_key);
    auto child_key_buffer =
        memory.loadN(child_storage_key_ptr, child_storage_key_size);
    auto prefixed_child_key = make_prefixed_child_storage_key(child_key_buffer);

    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);

    SL_TRACE_VOID_FUNC_CALL(logger_, child_key_buffer, key_buffer);

    auto inner_batch = storage_provider_->getChildBatchAt(prefixed_child_key);
    auto cursor = inner_batch.value()->trieCursor();
    auto seek_result = cursor->seekUpperBound(key_buffer);
    if (seek_result.has_error()) {
      logger_->error(
          "ext_default_child_storage_next_key_version_1 resulted with error: "
          "{}",
          seek_result.error().message());
      return kErrorSpan;
    }
    auto next_key_opt = cursor->key();
    if (auto enc_res = scale::encode(next_key_opt); enc_res.has_value()) {
      SL_TRACE_FUNC_CALL(logger_,
                         next_key_opt.has_value()
                             ? next_key_opt.value()
                             : common::Buffer().put("no value"),
                         child_key_buffer,
                         key_buffer);
      return memory.storeBuffer(enc_res.value());
    } else {  // NOLINT(readability-else-after-return)
      logger_->error(
          "ext_default_child_storage_next_key_version_1 result encoding "
          "resulted with error: {}",
          enc_res.error().message());
    }
    return kErrorSpan;
  }

  runtime::WasmSpan
  ChildStorageExtension::ext_default_child_storage_root_version_1(
      runtime::WasmSpan child_storage_key) const {
    outcome::result<storage::trie::RootHash> res{{}};
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [child_storage_key_ptr, child_storage_key_size] =
        runtime::PtrSize(child_storage_key);
    auto child_key_buffer =
        memory.loadN(child_storage_key_ptr, child_storage_key_size);
    auto prefixed_child_key = make_prefixed_child_storage_key(child_key_buffer);

    SL_TRACE_VOID_FUNC_CALL(logger_, child_key_buffer);

    if (auto opt_batch = storage_provider_->getChildBatchAt(prefixed_child_key);
        opt_batch.has_value() and opt_batch.value() != nullptr) {
      res = opt_batch.value()->commit();
    } else {
      logger_->warn(
          "ext_default_child_storage_root called in an ephemeral extension");
      res = storage_provider_->forceCommit();
      storage_provider_->getChildBatches().clear();
    }
    if (res.has_error()) {
      logger_->error(
          "ext_default_child_storage_root resulted with an error: {}",
          res.error().message());
    }
    const auto &root = res.value();
    return memory.storeBuffer(root);
  }

}  // namespace kagome::host_api
