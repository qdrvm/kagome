/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/offchain_extension.hpp"

#include <stdexcept>
#include <thread>

#include "runtime/memory.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/ptr_size.hpp"
#include "scale/scale.hpp"

namespace kagome::host_api {

  OffchainExtension::OffchainExtension(
      const application::AppConfiguration &app_config,
      std::shared_ptr<clock::SystemClock> system_clock,
      std::shared_ptr<offchain::OffchainStorage> storage,
      std::shared_ptr<const runtime::MemoryProvider> memory_provider,
      std::shared_ptr<crypto::CSPRNG> random_generator)
      : app_config_(app_config),
        system_clock_(std::move(system_clock)),
        storage_(std::move(storage)),
        memory_provider_(std::move(memory_provider)),
        random_generator_(std::move(random_generator)),
        log_(log::createLogger("OffchainExtension", "host_api")) {
    BOOST_ASSERT(system_clock_);
    BOOST_ASSERT(storage_);
    BOOST_ASSERT(memory_provider_);
    BOOST_ASSERT(random_generator_);
  }

  runtime::WasmI8 OffchainExtension::ext_offchain_is_validator_version_1() {
    bool isValidator = app_config_.roles().flags.authority == 1;
    return isValidator ? 1 : 0;
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_submit_transaction_version_1(
      runtime::WasmSpan data) {
    // TODO(xDimon): Need to implement it
    SL_DEBUG(log_,
             "Called "
             "OffchainExtension::ext_offchain_submit_transaction_version_1()");
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  runtime::WasmSpan OffchainExtension::ext_offchain_network_state_version_1() {
    // TODO(xDimon): Need to implement it
    SL_DEBUG(
        log_,
        "Called OffchainExtension::ext_offchain_network_state_version_1()");
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  runtime::WasmU64 OffchainExtension::ext_offchain_timestamp_version_1() {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  system_clock_->now().time_since_epoch())
                  .count();
    return ms;
  }

  void OffchainExtension::ext_offchain_sleep_until_version_1(
      runtime::WasmU64 deadline) {
    auto ts = system_clock_->zero() + std::chrono::milliseconds(deadline);

    SL_TRACE(log_,
             "Falling asleep until {} (for {}ms)",
             deadline,
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 ts - system_clock_->now())
                 .count());

    std::this_thread::sleep_until(ts);

    SL_DEBUG(log_, "Woke up after sleeping");
  }

  runtime::WasmPointer OffchainExtension::ext_offchain_random_seed_version_1() {
    auto &memory = memory_provider_->getCurrentMemory().value();

    std::array<uint8_t, 32> seed_bytes;
    random_generator_->fillRandomly(seed_bytes);

    return memory.storeBuffer(scale::encode(seed_bytes).value());
  }

  boost::optional<offchain::OffchainStorage &> OffchainExtension::getStorage(
      runtime::WasmI32 kind) {
    // TODO(xDimon): Need to implement it
    if (kind == 1) {  // Persistent
      return *storage_;
    }
    if (kind == 2) {  // Local
      return *storage_;
    }
    return boost::none;
  }

  void OffchainExtension::ext_offchain_local_storage_set_version_1(
      runtime::WasmI32 kind, runtime::WasmSpan key, runtime::WasmSpan value) {
    auto &memory = memory_provider_->getCurrentMemory().value();
    auto &storage = getStorage(kind).value();

    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);
    auto [value_ptr, value_size] = runtime::PtrSize(value);
    auto value_buffer = memory.loadN(value_ptr, value_size);

    auto result = storage.set(key_buffer, value_buffer);

    if (not result.has_failure()) {
      SL_TRACE_VOID_FUNC_CALL(log_, kind, key_buffer, value_buffer);

    } else {
      SL_TRACE(log_,
               "ext_offchain_local_storage_set_version_1( {}, {} ) => "
               "value was not modified. Reason: {}",
               key_buffer.toHex(),
               value_buffer.toHex(),
               result.error().message());
    }
  }

  void OffchainExtension::ext_offchain_local_storage_clear_version_1(
      runtime::WasmI32 kind, runtime::WasmSpan key) {
    auto &memory = memory_provider_->getCurrentMemory().value();
    auto &storage = getStorage(kind).value();

    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);

    auto result = storage.clear(key_buffer);

    if (result.has_value()) {
      SL_TRACE_VOID_FUNC_CALL(log_, kind, key_buffer);

    } else {
      SL_TRACE(log_,
               "ext_offchain_local_storage_clear_version_1( {} ) => "
               "value was not removed. Reason: {}",
               key_buffer.toHex(),
               result.error().message());
    }
  }

  runtime::WasmI8
  OffchainExtension::ext_offchain_local_storage_compare_and_set_version_1(
      runtime::WasmI32 kind,
      runtime::WasmSpan key,
      runtime::WasmSpan expected,
      runtime::WasmSpan value) {
    auto &memory = memory_provider_->getCurrentMemory().value();
    auto &storage = getStorage(kind).value();

    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);
    auto [expected_ptr, expected_size] = runtime::PtrSize(expected);
    auto expected_buffer = memory.loadN(expected_ptr, expected_size);
    auto [value_ptr, value_size] = runtime::PtrSize(value);
    auto value_buffer = memory.loadN(value_ptr, value_size);

    auto result =
        storage.compare_and_set(key_buffer, expected_buffer, value_buffer);
    auto option = result ? boost::make_optional(result.value()) : boost::none;

    if (option) {
      SL_TRACE_FUNC_CALL(log_,
                         option.value(),
                         kind,
                         key_buffer,
                         expected_buffer,
                         value_buffer);

    } else {
      SL_TRACE(log_,
               "ext_offchain_local_storage_compare_and_set_version_1"
               "( {}, {}, {} ) => value was not modified. Reason: {}",
               key_buffer.toHex(),
               expected_buffer.toHex(),
               value_buffer.toHex(),
               result.error().message());
    }

    return memory.storeBuffer(scale::encode(option).value());
  }

  runtime::WasmSpan OffchainExtension::ext_offchain_local_storage_get_version_1(
      runtime::WasmI32 kind, runtime::WasmSpan key) {
    auto &memory = memory_provider_->getCurrentMemory().value();
    auto &storage = getStorage(kind).value();

    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);

    auto result = storage.get(key_buffer);
    auto option = result ? boost::make_optional(result.value()) : boost::none;

    if (option) {
      SL_TRACE_FUNC_CALL(log_, option.value(), kind, key_buffer);

    } else {
      SL_TRACE(log_,
               "ext_offchain_local_storage_get_version_1( {} ) => "
               "value was not obtained. Reason: {}",
               key_buffer.toHex(),
               result.error().message());
    }

    return memory.storeBuffer(scale::encode(option).value());
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_request_start_version_1(
      runtime::WasmSpan method, runtime::WasmSpan uri, runtime::WasmSpan meta) {
    // TODO(xDimon): Need to implement it
    SL_DEBUG(log_,
             "Called "
             "OffchainExtension::ext_offchain_http_request_start_version_1()");
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_request_add_header_version_1(
      runtime::WasmI32 request_id,
      runtime::WasmSpan name,
      runtime::WasmSpan value) {
    // TODO(xDimon): Need to implement it
    SL_DEBUG(
        log_,
        "Called "
        "OffchainExtension::ext_offchain_http_request_add_header_version_1()");
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_request_write_body_version_1(
      runtime::WasmI32 request_id,
      runtime::WasmSpan chunk,
      runtime::WasmSpan deadline) {
    // TODO(xDimon): Need to implement it
    SL_DEBUG(
        log_,
        "Called "
        "OffchainExtension::ext_offchain_http_request_write_body_version_1()");
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }
  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_response_wait_version_1(
      runtime::WasmSpan ids, runtime::WasmSpan deadline) {
    // TODO(xDimon): Need to implement it
    SL_DEBUG(log_,
             "Called "
             "OffchainExtension::ext_offchain_http_response_wait_version_1()");
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_response_headers_version_1(
      runtime::WasmI32 request_id) {
    // TODO(xDimon): Need to implement it
    SL_DEBUG(
        log_,
        "Called "
        "OffchainExtension::ext_offchain_http_response_headers_version_1()");
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_response_read_body_version_1(
      runtime::WasmI32 request_id,
      runtime::WasmSpan buffer,
      runtime::WasmSpan deadline) {
    // TODO(xDimon): Need to implement it
    SL_DEBUG(
        log_,
        "Called "
        "OffchainExtension::ext_offchain_http_response_read_body_version_1()");
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  void OffchainExtension::ext_offchain_set_authorized_nodes_version_1(
      runtime::WasmSpan nodes, runtime::WasmI32 authorized_only) {
    // TODO(xDimon): Need to implement it
    SL_DEBUG(
        log_,
        "Called "
        "OffchainExtension::ext_offchain_set_authorized_nodes_version_1()");
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
  }

  void OffchainExtension::ext_offchain_index_set_version_1(
      runtime::WasmSpan key, runtime::WasmSpan value) {
    // TODO(xDimon): Need to implement it
    SL_DEBUG(log_,
             "Called OffchainExtension::ext_offchain_index_set_version_1()");
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
  }

  void OffchainExtension::ext_offchain_index_clear_version_1(
      runtime::WasmSpan key) {
    // TODO(xDimon): Need to implement it
    SL_DEBUG(log_,
             "Called OffchainExtension::ext_offchain_index_clear_version_1()");
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
  }

}  // namespace kagome::host_api
