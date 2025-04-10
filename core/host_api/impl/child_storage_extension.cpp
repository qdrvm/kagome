/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/child_storage_extension.hpp"

#include <tuple>
#include <utility>

#include <fmt/std.h>

#include "common/monadic_utils.hpp"
#include "host_api/impl/storage_util.hpp"
#include "log/trace_macros.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/ptr_size.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "scale/encode_append.hpp"
#include "scale/kagome_scale.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

using kagome::common::Buffer;
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
    return Buffer{storage::kChildStorageDefaultPrefix}.put(child_storage_key);
  }

  template <typename R, typename F, typename... Args>
  outcome::result<R> ChildStorageExtension::executeOnConstChildStorage(
      const Buffer &child_storage_key, F func, Args &&...args) const {
    OUTCOME_TRY(prefixed_child_key,
                make_prefixed_child_storage_key(child_storage_key));
    OUTCOME_TRY(child_batch,
                storage_provider_->getChildBatchAt(prefixed_child_key));

    return func(child_batch.get(), std::forward<Args>(args)...);
  }

  template <typename R, typename F, typename... Args>
  outcome::result<R> ChildStorageExtension::executeOnMutChildStorage(
      const Buffer &child_storage_key, F func, Args &&...args) const {
    OUTCOME_TRY(prefixed_child_key,
                make_prefixed_child_storage_key(child_storage_key));
    OUTCOME_TRY(child_batch,
                storage_provider_->getMutableChildBatchAt(prefixed_child_key));

    return func(child_batch.get(), std::forward<Args>(args)...);
  }

  template <typename Arg>
  auto loadBuffer(runtime::Memory &memory, Arg &&span) {
    auto [span_ptr, span_size] = runtime::PtrSize(std::forward<Arg>(span));
    return memory.loadN(span_ptr, span_size);
  }

  template <typename... Args>
  auto loadBuffer(runtime::Memory &memory, const Args &...spans) {
    return std::make_tuple(loadBuffer(memory, spans)...);
  }

  void ChildStorageExtension::ext_default_child_storage_set_version_1(
      runtime::WasmSpan child_storage_key,
      runtime::WasmSpan key,
      runtime::WasmSpan value) {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto child_key_buffer = loadBuffer(memory, child_storage_key);
    auto key_buffer = loadBuffer(memory, key);
    auto value_buffer = loadBuffer(memory, value);

    SL_TRACE_VOID_FUNC_CALL(
        logger_, child_key_buffer, key_buffer, value_buffer);

    auto result = executeOnMutChildStorage<void>(
        child_key_buffer, [&](auto &child_batch) mutable {
          return child_batch.put(key_buffer, std::move(value_buffer));
        });

    if (not result) {
      logger_->error(
          "ext_default_child_storage_set_version_1 failed with reason: {}",
          result.error());
    }
  }

  runtime::WasmSpan
  ChildStorageExtension::ext_default_child_storage_get_version_1(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan key) const {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto child_key_buffer = loadBuffer(memory, child_storage_key);
    auto key_buffer = loadBuffer(memory, key);

    SL_TRACE_VOID_FUNC_CALL(logger_, child_key_buffer, key_buffer);

    return executeOnConstChildStorage<runtime::WasmSpan>(
               child_key_buffer,
               [&memory, &child_key_buffer, &key_buffer, this](
                   auto &child_batch, auto &key) {
                 constexpr auto error_message =
                     "ext_default_child_storage_get_version_1( {}, {} ) => "
                     "value was not "
                     "obtained. Reason: {}";

                 auto result = child_batch.tryGet(key);
                 if (result) {
                   SL_TRACE_FUNC_CALL(
                       logger_, result.value(), child_key_buffer, key_buffer);
                 } else {
                   logger_->error(error_message,
                                  child_key_buffer.toHex(),
                                  key_buffer.toHex(),
                                  result.error());
                 }

                 // okay to throw, we want to end this runtime call with error
                 return memory.storeBuffer(
                     scale::encode(
                         common::map_optional(result.value(),
                                              [](auto &r) { return r.view(); }))
                         .value());
               },
               key_buffer)
        .value();
  }

  void ChildStorageExtension::ext_default_child_storage_clear_version_1(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan key) {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [child_key_buffer, key_buffer] =
        loadBuffer(memory, child_storage_key, key);

    SL_TRACE_VOID_FUNC_CALL(logger_, child_key_buffer, key_buffer);

    auto result = executeOnMutChildStorage<void>(
        child_key_buffer,
        [](auto &child_batch, auto &key) { return child_batch.remove(key); },
        key_buffer);

    if (not result) {
      logger_->error(
          "ext_default_child_storage_clear_version_1 failed, due to fail in "
          "trie "
          "db with reason: {}",
          result.error());
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
        storage_provider_->getMutableChildBatchAt(prefixed_child_key);
    if (child_batch_outcome.has_error()) {
      logger_->error(
          "ext_default_child_storage_next_key_version_1 resulted with error: "
          "{}",
          child_batch_outcome.error());
      return kErrorSpan;
    }
    auto &child_batch = child_batch_outcome.value().get();
    auto cursor = child_batch.trieCursor();

    auto seek_result = cursor->seekUpperBound(key_buffer);
    if (seek_result.has_error()) {
      logger_->error(
          "ext_default_child_storage_next_key_version_1 resulted with error: "
          "{}",
          seek_result.error());
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
          enc_res.error());
    }
    return kErrorSpan;
  }

  Hash256 ChildStorageExtension::ext_default_child_storage_root_version_1(
      BufferView child) const {
    return ext_default_child_storage_root_version_2(
        child, storage::trie::StateVersion::V0);
  }

  Hash256 ChildStorageExtension::ext_default_child_storage_root_version_2(
      BufferView child, storage::trie::StateVersion version) const {
    return storage_provider_->commit(child, version).value();
  }

  void ChildStorageExtension::ext_default_child_storage_clear_prefix_version_1(
      BufferView child, BufferView prefix) {
    storage_provider_->clearPrefix(child, prefix, std::nullopt);
  }

  KillStorageResult
  ChildStorageExtension::ext_default_child_storage_clear_prefix_version_2(
      BufferView child, BufferView prefix, ClearPrefixLimit limit) {
    return storage_provider_->clearPrefix(child, prefix, limit);
  }

  runtime::WasmSpan
  ChildStorageExtension::ext_default_child_storage_read_version_1(
      runtime::WasmSpan child_storage_key,
      runtime::WasmSpan key,
      runtime::WasmSpan value_out,
      runtime::WasmOffset offset) const {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [child_key_buffer, key_buffer] =
        loadBuffer(memory, child_storage_key, key);
    auto [value_ptr, value_size] = runtime::PtrSize(value_out);

    auto value =
        executeOnConstChildStorage<std::optional<common::BufferOrView>>(
            child_key_buffer,
            [](auto &child_batch, auto &key) {
              return child_batch.tryGet(key);
            },
            key_buffer);
    std::optional<uint32_t> res{std::nullopt};
    if (value) {
      auto &data_opt = value.value();
      if (data_opt.has_value()) {
        common::BufferView data = data_opt.value();
        data = data.subspan(std::min<size_t>(offset, data.size()));
        auto written = std::min<size_t>(data.size(), value_size);
        memory.storeBuffer(value_ptr, data.first(written));
        res = data.size();

        SL_TRACE_FUNC_CALL(logger_,
                           data,
                           child_key_buffer,
                           key,
                           common::Buffer{data.first(written)});
      } else {
        SL_TRACE_FUNC_CALL(logger_,
                           std::string_view{"none"},
                           child_key_buffer,
                           key,
                           value_out,
                           offset);
      }
    } else {
      SL_ERROR(
          logger_, "Error in ext_storage_read_version_1: {}", value.error());
      value.value();
    }

    return memory.storeBuffer(scale::encode(res).value());
  }

  int32_t ChildStorageExtension::ext_default_child_storage_exists_version_1(
      runtime::WasmSpan child_storage_key, runtime::WasmSpan key) const {
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto [child_key_buffer, key_buffer] =
        loadBuffer(memory, child_storage_key, key);

    SL_TRACE_VOID_FUNC_CALL(logger_, child_key_buffer, key_buffer);

    auto res = executeOnConstChildStorage<bool>(
        child_key_buffer,
        [](auto &child_batch, auto &key) { return child_batch.contains(key); },
        key_buffer);

    if (not res) {
      logger_->error(
          "ext_default_child_storage_exists_version_1 failed with "
          "reason: {}",
          res.error());
    }

    return (res.has_value() and res.value()) ? 1 : 0;
  }

  void ChildStorageExtension::ext_default_child_storage_storage_kill_version_1(
      BufferView child) {
    storage_provider_->clearPrefix(child, {}, std::nullopt);
  }

  KillStorageResult
  ChildStorageExtension::ext_default_child_storage_storage_kill_version_3(
      BufferView child, ClearPrefixLimit limit) {
    return storage_provider_->clearPrefix(child, {}, limit);
  }

}  // namespace kagome::host_api
