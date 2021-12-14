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
#include "storage/predefined_keys.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

#include <tuple>
#include <utility>

using kagome::common::Buffer;
using kagome::common::BufferConstRef;
using kagome::storage::trie::TrieError;

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

  outcome::result<Buffer> make_prefixed_child_storage_key(
      const Buffer &child_storage_key) {
    return Buffer{storage::kChildStorageDefaultPrefix}.putBuffer(
        child_storage_key);
  }

  template <typename R, typename F, typename... Args>
  outcome::result<R> ChildStorageExtension::executeOnChildStorage(
      const Buffer &child_storage_key, F func, Args &&...args) const {
    OUTCOME_TRY(prefixed_child_key,
                make_prefixed_child_storage_key(child_storage_key));
    OUTCOME_TRY(child_batch,
                storage_provider_->getChildBatchAt(prefixed_child_key));

    auto result = func(child_batch, std::forward<Args>(args)...);
    if (not result) {
      storage_provider_->clearChildBatches();
      return result;
    }

    OUTCOME_TRY(new_child_root, child_batch->commit());
    OUTCOME_TRY(storage_provider_->getCurrentBatch()->put(
        prefixed_child_key, Buffer{scale::encode(new_child_root).value()}));
    storage_provider_->clearChildBatches();
    if constexpr (!std::is_void_v<R>) {
      return result;
    }
    return outcome::success();
  }

  template <typename... Args>
  auto ChildStorageExtension::loadBuffer(runtime::Memory &memory,
                                         Args &&...spans) const {
    auto f = [&memory](auto &&span) {
      auto [span_ptr, span_size] = runtime::PtrSize(span);
      return memory.loadN(span_ptr, span_size);
    };
    return std::make_tuple(f(std::forward<Args>(spans))...);
  }

  void ChildStorageExtension::ext_default_child_storage_set_version_1(
      runtime::WasmSpan child_storage_key,
      runtime::WasmSpan key,
      runtime::WasmSpan value) {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [child_key_buffer, key_buffer, value_buffer] =
        loadBuffer(memory, child_storage_key, key, value);

    SL_TRACE_VOID_FUNC_CALL(
        logger_, child_key_buffer, key_buffer, value_buffer);

    auto result = executeOnChildStorage<void>(
        child_key_buffer,
        [](auto &child_batch, auto &key, auto &value) {
          return child_batch->put(key, value);
        },
        key_buffer,
        value_buffer);

    if (not result) {
      logger_->error(
          "ext_default_child_storage_set_version_1 failed with reason: {}",
          result.error().message());
    }
  }

  runtime::WasmSpan
  ChildStorageExtension::ext_default_child_storage_get_version_1(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan key) const {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [child_key_buffer, key_buffer] =
        loadBuffer(memory, child_storage_key, key);
    constexpr auto error_message =
        "ext_default_child_storage_get_version_1( {}, {} ) => value was not "
        "obtained. Reason: {}";

    SL_TRACE_VOID_FUNC_CALL(logger_, child_key_buffer, key_buffer);

    auto result = executeOnChildStorage<std::optional<BufferConstRef>>(
        child_key_buffer,
        [](auto &child_batch, auto &key) { return child_batch->tryGet(key); },
        key_buffer);

    if (result) {
      SL_TRACE_FUNC_CALL(logger_, result.value(), child_key_buffer, key_buffer);
    } else {
      logger_->error(error_message,
                     child_key_buffer.toHex(),
                     key_buffer.toHex(),
                     result.error().message());
    }
    return memory.storeBuffer(scale::encode(result.value()).value());
  }

  void ChildStorageExtension::ext_default_child_storage_clear_version_1(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan key) {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [child_key_buffer, key_buffer] =
        loadBuffer(memory, child_storage_key, key);

    SL_TRACE_VOID_FUNC_CALL(logger_, child_key_buffer, key_buffer);

    auto result = executeOnChildStorage<void>(
        child_key_buffer,
        [](auto &child_batch, auto &key) { return child_batch->remove(key); },
        key_buffer);

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
    auto [child_key_buffer, key_buffer] =
        loadBuffer(memory, child_storage_key, key);
    auto prefixed_child_key =
        make_prefixed_child_storage_key(child_key_buffer).value();

    SL_TRACE_VOID_FUNC_CALL(logger_, child_key_buffer, key_buffer);

    auto child_batch_outcome =
        storage_provider_->getChildBatchAt(prefixed_child_key);
    if (child_batch_outcome.has_error()) {
      logger_->error(
          "ext_default_child_storage_next_key_version_1 resulted with error: "
          "{}",
          child_batch_outcome.error().message());
      storage_provider_->clearChildBatches();
      return kErrorSpan;
    }
    auto child_batch = child_batch_outcome.value();
    auto cursor = child_batch->trieCursor();
    storage_provider_->clearChildBatches();
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
                         next_key_opt.has_value() ? next_key_opt.value()
                                                  : Buffer().put("no value"),
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
    auto [child_key_buffer] = loadBuffer(memory, child_storage_key);
    auto prefixed_child_key = make_prefixed_child_storage_key(child_key_buffer);

    SL_TRACE_VOID_FUNC_CALL(logger_, child_key_buffer);

    if (auto child_batch =
            storage_provider_->getChildBatchAt(prefixed_child_key.value());
        child_batch.has_value() and child_batch.value() != nullptr) {
      res = child_batch.value()->commit();
    } else {
      logger_->warn(
          "ext_default_child_storage_root called in an ephemeral extension");
      res = storage_provider_->forceCommit();
      storage_provider_->clearChildBatches();
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
