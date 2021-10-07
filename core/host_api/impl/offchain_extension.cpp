/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/offchain_extension.hpp"

#include "stdexcept"

#include "runtime/runtime_api/offchain_api.hpp"

namespace kagome::host_api {

  OffchainExtension::OffchainExtension(
      std::shared_ptr<runtime::OffchainApi> offchain_api)
      : offchain_api_(std::move(offchain_api)) {}

  runtime::WasmI8 OffchainExtension::ext_offchain_is_validator_version_1() {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_submit_transaction_version_1(
      runtime::WasmSpan data) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  runtime::WasmSpan OffchainExtension::ext_offchain_network_state_version_1() {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  runtime::WasmU64 OffchainExtension::ext_offchain_timestamp_version_1() {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  void OffchainExtension::ext_offchain_sleep_until_version_1(
      runtime::WasmU64 deadline) {}
  runtime::WasmPointer OffchainExtension::ext_offchain_random_seed_version_1() {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  void OffchainExtension::ext_offchain_local_storage_set_version_1(
      runtime::WasmI32 kind, runtime::WasmSpan key, runtime::WasmSpan value) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
  }

  void OffchainExtension::ext_offchain_local_storage_clear_version_1(
      runtime::WasmI32 kind, runtime::WasmSpan key) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
  }

  runtime::WasmI8
  OffchainExtension::ext_offchain_local_storage_compare_and_set_version_1(
      runtime::WasmI32 kind,
      runtime::WasmSpan key,
      runtime::WasmSpan expected,
      runtime::WasmSpan value) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  runtime::WasmSpan OffchainExtension::ext_offchain_local_storage_get_version_1(
      runtime::WasmI32 kind, runtime::WasmSpan key) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_request_start_version_1(
      runtime::WasmSpan method, runtime::WasmSpan uri, runtime::WasmSpan meta) {
    // TODO(xDimon): Need to implement it
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
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }
  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_response_wait_version_1(
      runtime::WasmSpan ids, runtime::WasmSpan deadline) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_response_headers_version_1(
      runtime::WasmI32 request_id) {
    // TODO(xDimon): Need to implement it
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
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
    return 0;
  }

  void OffchainExtension::ext_offchain_set_authorized_nodes_version_1(
      runtime::WasmSpan nodes, runtime::WasmI32 authorized_only) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
  }

  void OffchainExtension::ext_offchain_index_set_version_1(
      runtime::WasmSpan key, runtime::WasmSpan value) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
  }

  void OffchainExtension::ext_offchain_index_clear_version_1(
      runtime::WasmSpan key) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainExtension is not implemented yet");
  }

}  // namespace kagome::host_api
