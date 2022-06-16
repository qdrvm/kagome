/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/offchain_extension.hpp"

#include <stdexcept>
#include <thread>

#include "offchain/offchain_worker.hpp"
#include "offchain/offchain_worker_pool.hpp"
#include "runtime/memory.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/ptr_size.hpp"
#include "scale/scale.hpp"

namespace kagome::host_api {

  using offchain::HttpMethod;
  using offchain::OffchainWorker;
  using offchain::RequestId;
  using offchain::StorageType;
  using offchain::Timestamp;

  OffchainExtension::OffchainExtension(
      const OffchainExtensionConfig &config,
      std::shared_ptr<const runtime::MemoryProvider> memory_provider,
      std::shared_ptr<offchain::OffchainStorage> offchain_storage,
      std::shared_ptr<offchain::OffchainWorkerPool> ocw_pool)
      : config_(config),
        memory_provider_(std::move(memory_provider)),
        offchain_storage_(std::move(offchain_storage)),
        ocw_pool_(std::move(ocw_pool)),
        log_(log::createLogger("OffchainExtension", "offchain_extension")) {
    BOOST_ASSERT(memory_provider_);
    BOOST_ASSERT(offchain_storage_);
    BOOST_ASSERT(ocw_pool_);
  }

  std::shared_ptr<offchain::OffchainWorker> OffchainExtension::getWorker() {
    auto worker_opt = ocw_pool_->getWorker();
    if (not worker_opt.has_value()) {
      throw std::runtime_error("Method was called not in offchain worker context");
    }
    return worker_opt.value();
  }

  runtime::WasmI32 OffchainExtension::ext_offchain_is_validator_version_1() {
    auto worker = getWorker();
    bool isValidator = worker->isValidator();
    return isValidator ? 1 : 0;
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_submit_transaction_version_1(
      runtime::WasmSpan data_pos) {
    auto worker = getWorker();
    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto [data_ptr, data_size] = runtime::PtrSize(data_pos);
    auto data_buffer = memory.loadN(data_ptr, data_size);
    auto xt_res = scale::decode<primitives::Extrinsic>(data_buffer);
    if (xt_res.has_error()) {
      throw std::runtime_error("Invalid encoded data for transaction arg");
    }
    auto &xt = xt_res.value();

    auto result = worker->submitTransaction(std::move(xt));

    return memory.storeBuffer(scale::encode(result).value());
  }

  runtime::WasmSpan OffchainExtension::ext_offchain_network_state_version_1() {
    auto worker = getWorker();
    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto result = worker->networkState();

    return memory.storeBuffer(scale::encode(result).value());
  }

  runtime::WasmU64 OffchainExtension::ext_offchain_timestamp_version_1() {
    auto worker = getWorker();
    auto result = worker->timestamp();
    return result;
  }

  void OffchainExtension::ext_offchain_sleep_until_version_1(
      runtime::WasmU64 deadline) {
    auto worker = getWorker();
    worker->sleepUntil(deadline);
  }

  runtime::WasmPointer OffchainExtension::ext_offchain_random_seed_version_1() {
    auto worker = getWorker();
    auto &memory = memory_provider_->getCurrentMemory()->get();
    auto result = worker->timestamp();
    return memory.storeBuffer(scale::encode(result).value());
  }

  void OffchainExtension::ext_offchain_local_storage_set_version_1(
      runtime::WasmI32 kind, runtime::WasmSpan key, runtime::WasmSpan value) {
    auto worker = getWorker();
    auto &memory = memory_provider_->getCurrentMemory()->get();

    StorageType storage_type = StorageType::Undefined;
    if (kind == 1) {
      storage_type = StorageType::Persistent;
    } else if (kind == 2) {
      storage_type = StorageType::Local;
    } else if (kind == 0) {
      // TODO(xDimon): Remove this if-branch when it will be fixed it substrate
      //  issue: https://github.com/soramitsu/kagome/issues/997
      storage_type = StorageType::Persistent;
    } else {
      throw std::invalid_argument(
          "Method was called with unknown kind of storage");
    }

    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);
    auto [value_ptr, value_size] = runtime::PtrSize(value);
    auto value_buffer = memory.loadN(value_ptr, value_size);

    worker->localStorageSet(storage_type, key_buffer, value_buffer);
  }

  void OffchainExtension::ext_offchain_local_storage_clear_version_1(
      runtime::WasmI32 kind, runtime::WasmSpan key) {
    auto worker = getWorker();
    auto &memory = memory_provider_->getCurrentMemory()->get();

    StorageType storage_type = StorageType::Undefined;
    if (kind == 1) {
      storage_type = StorageType::Persistent;
    } else if (kind == 2) {
      storage_type = StorageType::Local;
    } else if (kind == 0) {
      // TODO(xDimon): Remove this if-branch when it will be fixed it substrate
      //  issue: https://github.com/soramitsu/kagome/issues/997
      storage_type = StorageType::Persistent;
    } else {
      throw std::invalid_argument(
          "Method was called with unknown kind of storage");
    }

    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);

    worker->localStorageClear(storage_type, key_buffer);
  }

  runtime::WasmI32
  OffchainExtension::ext_offchain_local_storage_compare_and_set_version_1(
      runtime::WasmI32 kind,
      runtime::WasmSpan key,
      runtime::WasmSpan expected,
      runtime::WasmSpan value) {
    auto worker = getWorker();
    auto &memory = memory_provider_->getCurrentMemory()->get();

    StorageType storage_type = StorageType::Undefined;
    if (kind == 1) {
      storage_type = StorageType::Persistent;
    } else if (kind == 2) {
      storage_type = StorageType::Local;
    } else if (kind == 0) {
      // TODO(xDimon): Remove this if-branch when it will be fixed it substrate
      //  issue: https://github.com/soramitsu/kagome/issues/997
      storage_type = StorageType::Persistent;
    } else {
      throw std::invalid_argument(
          "Method was called with unknown kind of storage");
    }

    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);
    auto [expected_ptr, expected_size] = runtime::PtrSize(expected);
    auto expected_buffer = memory.loadN(expected_ptr, expected_size);
    auto [value_ptr, value_size] = runtime::PtrSize(value);
    auto value_buffer = memory.loadN(value_ptr, value_size);

    auto result = worker->localStorageCompareAndSet(
        storage_type, key_buffer, expected_buffer, value_buffer);

    return result;
  }

  runtime::WasmSpan OffchainExtension::ext_offchain_local_storage_get_version_1(
      runtime::WasmI32 kind, runtime::WasmSpan key) {
    auto worker = getWorker();
    auto &memory = memory_provider_->getCurrentMemory()->get();

    StorageType storage_type = StorageType::Undefined;
    if (kind == 1) {
      storage_type = StorageType::Persistent;
    } else if (kind == 2) {
      storage_type = StorageType::Local;
    } else if (kind == 0) {
      // TODO(xDimon): Remove this if-branch when it will be fixed it substrate
      //  issue: https://github.com/soramitsu/kagome/issues/997
      storage_type = StorageType::Persistent;
    } else {
      throw std::invalid_argument(
          "Method was called with unknown kind of storage");
    }

    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);

    auto result = worker->localStorageGet(storage_type, key_buffer);

    auto option = result ? std::make_optional(result.value()) : std::nullopt;

    return memory.storeBuffer(scale::encode(option).value());
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_request_start_version_1(
      runtime::WasmSpan method_pos,
      runtime::WasmSpan uri_pos,
      runtime::WasmSpan meta_pos) {
    auto worker = getWorker();
    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto [method_ptr, method_size] = runtime::PtrSize(method_pos);
    auto method_buffer = memory.loadN(method_ptr, method_size);

    auto [uri_ptr, uri_size] = runtime::PtrSize(uri_pos);
    auto uri_buffer = memory.loadN(uri_ptr, uri_size);
    auto uri = uri_buffer.toString();

    auto [meta_ptr, meta_size] = runtime::PtrSize(meta_pos);
    [[maybe_unused]]  // It is future-reserved field, is not used now
    auto meta_buffer = memory.loadN(meta_ptr, meta_size);

    HttpMethod method = HttpMethod::Undefined;
    if (method_buffer.toString() == "Get") {
      method = HttpMethod::Get;
    } else if (method_buffer.toString() == "Post") {
      method = HttpMethod::Post;
    } else {
      SL_TRACE(
          log_,
          "ext_offchain_http_request_start_version_1( {}, {}, {} ) failed: "
          "Reason: unknown method",
          method_buffer.toString(),
          uri,
          meta_buffer.toString());
    }

    auto result = worker->httpRequestStart(method, uri, meta_buffer);

    if (result.isSuccess()) {
      SL_TRACE_FUNC_CALL(
          log_, result.value(), method_buffer.toString(), uri, meta_buffer);

    } else {
      SL_TRACE(log_,
               "ext_offchain_http_request_start_version_1( {}, {}, {} ) failed "
               "during execution",
               method_buffer.toString(),
               uri,
               meta_buffer.toString());
    }

    return memory.storeBuffer(scale::encode(result).value());
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_request_add_header_version_1(
      runtime::WasmI32 request_id,
      runtime::WasmSpan name_pos,
      runtime::WasmSpan value_pos) {
    auto worker = getWorker();

    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto [name_ptr, name_size] = runtime::PtrSize(name_pos);
    auto name_buffer = memory.loadN(name_ptr, name_size);
    auto name = name_buffer.toString();

    auto [value_ptr, value_size] = runtime::PtrSize(value_pos);
    auto value_buffer = memory.loadN(value_ptr, value_size);
    auto value = value_buffer.toString();

    auto result = worker->httpRequestAddHeader(request_id, name, value);

    if (result.isSuccess()) {
      SL_TRACE_FUNC_CALL(log_, "Success", name, value);

    } else {
      SL_TRACE(
          log_,
          "ext_offchain_http_request_add_header_version_1( {}, {}, {} ) failed "
          "during execution",
          request_id,
          name,
          value);
    }

    return memory.storeBuffer(scale::encode(result).value());
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_request_write_body_version_1(
      runtime::WasmI32 request_id,
      runtime::WasmSpan chunk_pos,
      runtime::WasmSpan deadline_pos) {
    auto worker = getWorker();

    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto [chunk_ptr, chunk_size] = runtime::PtrSize(chunk_pos);
    auto chunk_buffer = memory.loadN(chunk_ptr, chunk_size);

    auto [deadline_ptr, deadline_size] = runtime::PtrSize(deadline_pos);
    auto deadline_buffer = memory.loadN(deadline_ptr, deadline_size);
    auto deadline_res =
        scale::decode<std::optional<Timestamp>>(deadline_buffer);
    if (deadline_res.has_error()) {
      throw std::runtime_error("Invalid encoded data for deadline arg");
    }
    auto &deadline = deadline_res.value();

    auto result =
        worker->httpRequestWriteBody(request_id, chunk_buffer, deadline);

    return memory.storeBuffer(scale::encode(result).value());
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_response_wait_version_1(
      runtime::WasmSpan ids_pos, runtime::WasmSpan deadline_pos) {
    auto worker = getWorker();

    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto [ids_ptr, ids_size] = runtime::PtrSize(ids_pos);
    auto ids_buffer = memory.loadN(ids_ptr, ids_size);
    auto ids_res = scale::decode<std::vector<RequestId>>(ids_buffer);
    if (ids_res.has_error()) {
      throw std::runtime_error("Invalid encoded data for IDs arg");
    }
    auto &ids = ids_res.value();

    auto [deadline_ptr, deadline_size] = runtime::PtrSize(deadline_pos);
    auto deadline_buffer = memory.loadN(deadline_ptr, deadline_size);
    auto deadline_res =
        scale::decode<std::optional<Timestamp>>(deadline_buffer);
    if (deadline_res.has_error()) {
      throw std::runtime_error("Invalid encoded data for deadline arg");
    }
    auto &deadline = deadline_res.value();

    auto result = worker->httpResponseWait(ids, deadline);

    return memory.storeBuffer(scale::encode(result).value());
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_response_headers_version_1(
      runtime::WasmI32 request_id) {
    auto worker = getWorker();

    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto result = worker->httpResponseHeaders(request_id);

    SL_TRACE_FUNC_CALL(
        log_, fmt::format("<{} headers>", result.size()), request_id);

    return memory.storeBuffer(scale::encode(result).value());
  }

  runtime::WasmSpan
  OffchainExtension::ext_offchain_http_response_read_body_version_1(
      runtime::WasmI32 request_id,
      runtime::WasmSpan buffer_pos,
      runtime::WasmSpan deadline_pos) {
    auto worker = getWorker();
    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto dst_buffer = runtime::PtrSize(buffer_pos);

    auto [deadline_ptr, deadline_size] = runtime::PtrSize(deadline_pos);
    auto deadline_buffer = memory.loadN(deadline_ptr, deadline_size);
    auto deadline_res =
        scale::decode<std::optional<Timestamp>>(deadline_buffer);
    if (deadline_res.has_error()) {
      throw std::runtime_error("Invalid encoded data for deadline arg");
    }
    auto &deadline = deadline_res.value();

    common::Buffer buffer;
    buffer.resize(dst_buffer.size);

    auto result = worker->httpResponseReadBody(request_id, buffer, deadline);

    if (result.isSuccess()) {
      memory.storeBuffer(dst_buffer.ptr, buffer);
    }

    return memory.storeBuffer(scale::encode(result).value());
  }

  void OffchainExtension::ext_offchain_set_authorized_nodes_version_1(
      runtime::WasmSpan nodes_pos, runtime::WasmI32 authorized_only) {
    auto worker = getWorker();
    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto [nodes_ptr, nodes_size] = runtime::PtrSize(nodes_pos);
    auto nodes_buffer = memory.loadN(nodes_ptr, nodes_size);
    auto nodes_res = scale::decode<std::vector<common::Buffer>>(nodes_buffer);
    if (nodes_res.has_error()) {
      throw std::runtime_error("Invalid encoded data for nodes arg");
    }
    const auto &nodes_as_buffers = nodes_res.value();

    std::vector<libp2p::peer::PeerId> nodes;
    for (auto buff : nodes_as_buffers) {
      auto peer_id_res = libp2p::peer::PeerId::fromBytes(buff);
      if (peer_id_res.has_error()) {
        throw std::runtime_error("Invalid encoded data for nodes arg");
      }
      auto &peer_id = peer_id_res.value();
      nodes.emplace_back(std::move(peer_id));
    }

    worker->setAuthorizedNodes(std::move(nodes), authorized_only == 1);
  }

  void OffchainExtension::ext_offchain_index_set_version_1(
      runtime::WasmSpan key, runtime::WasmSpan value) {
    if (not config_.is_indexing_enabled) {
      return;
    }

    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);
    auto [value_ptr, value_size] = runtime::PtrSize(value);
    auto value_buffer = memory.loadN(value_ptr, value_size);

    auto result = offchain_storage_->set(key_buffer, std::move(value_buffer));
    if (result.has_error()) {
      SL_WARN(log_, "Can't set value in storage: {}", result.error().message());
    }
  }

  void OffchainExtension::ext_offchain_index_clear_version_1(
      runtime::WasmSpan key) {
    if (not config_.is_indexing_enabled) {
      return;
    }

    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto [key_ptr, key_size] = runtime::PtrSize(key);
    auto key_buffer = memory.loadN(key_ptr, key_size);

    auto result = offchain_storage_->clear(key_buffer);
    if (result.has_error()) {
      SL_WARN(
          log_, "Can't clear value in storage: {}", result.error().message());
    }
  }

}  // namespace kagome::host_api
