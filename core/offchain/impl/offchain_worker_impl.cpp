/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/offchain_worker_impl.hpp"

#include <thread>

#include "log/logger.hpp"
#include "runtime/executor.hpp"

namespace kagome::offchain {

  OffchainWorkerImpl::OffchainWorkerImpl(
      std::shared_ptr<runtime::Executor> executor,
      const primitives::BlockHash &block,
      const primitives::BlockHeader &header)
      : executor_(std::move(executor)),
        associated_block_(header.number, block),
        header_(header) {
    BOOST_ASSERT(executor_);
    io_context_.run();
  }

  outcome::result<void> OffchainWorkerImpl::run(
      std::shared_ptr<OffchainWorkerImpl> worker) {
    auto main_thread_func = [ocw = std::move(worker)] {
      soralog::util::setThreadName(
          "ocw." + std::to_string(ocw->associated_block_.number));

      current(*ocw);

      log::Logger log = log::createLogger(
          "OffchainWorker#" + std::to_string(ocw->associated_block_.number),
          "offchain_worker_api");

      SL_TRACE(log,
               "Offchain worker is started for block #{} hash={}",
               ocw->associated_block_.number,
               ocw->associated_block_.hash.toHex());

      auto res =
          ocw->executor_->callAt<void>(ocw->associated_block_.hash,
                                       "OffchainWorkerApi_offchain_worker",
                                       ocw->header_);

      current(boost::none);

      if (res.has_failure()) {
        log->error("Can't execute offchain worker for block #{} hash={}: {}",
                   ocw->associated_block_.number,
                   ocw->associated_block_.hash.toHex(),
                   res.error().message());
        return;
      }

      SL_DEBUG(log,
               "Offchain worker is successfully executed for block #{} hash={}",
               ocw->associated_block_.number,
               ocw->associated_block_.hash.toHex());
    };

    try {
      BOOST_ASSERT(worker == nullptr);
      std::thread(std::move(main_thread_func)).detach();
    } catch (const std::system_error &exception) {
      return outcome::failure(exception.code());
    } catch (...) {
      return outcome::failure(boost::system::error_code{});
    }

    return outcome::success();
  }

  bool OffchainWorkerImpl::isValidator() const {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return false;
  }

  common::Buffer OffchainWorkerImpl::submitTransaction(
      const primitives::Extrinsic &ext) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return {};
  }

  Result<OpaqueNetworkState, Failure> OffchainWorkerImpl::networkState() {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return Failure();
  }

  Timestamp OffchainWorkerImpl::offchainTimestamp() {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return {};
  }

  void OffchainWorkerImpl::sleepUntil(Timestamp) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return;
  }

  RandomSeed OffchainWorkerImpl::randomSeed() {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return {};
  }

  offchain::OffchainStorage &OffchainWorkerImpl::getStorage(
      StorageType storage_type) {
    switch (storage_type) {
      case StorageType::Persistent:
        return *persistent_storage_;
      case StorageType::Local:
        return *local_storage_;
      case StorageType::Undefined:
      default:
        BOOST_UNREACHABLE_RETURN({});
    }
  }

  void OffchainWorkerImpl::localStorageSet(StorageType storage_type,
                                           const common::Buffer &key,
                                           common::Buffer value) {
    auto &storage = getStorage(storage_type);
    auto result = storage.set(key, std::move(value));
    if (result.has_error()) {
      SL_WARN(log_, "Can't set value in storage: {}", result.error().message());
    }
  }

  void OffchainWorkerImpl::localStorageClear(StorageType storage_type,
                                             const common::Buffer &key) {
    auto &storage = getStorage(storage_type);
    auto result = storage.clear(key);
    if (result.has_error()) {
      SL_WARN(
          log_, "Can't clear value in storage: {}", result.error().message());
    }
  }

  bool OffchainWorkerImpl::localStorageCompareAndSet(
      StorageType storage_type,
      const common::Buffer &key,
      boost::optional<const common::Buffer &> expected,
      common::Buffer value) {
    auto &storage = getStorage(storage_type);
    auto result = storage.compare_and_set(key, expected, std::move(value));
    if (result.has_error()) {
      SL_WARN(log_,
              "Can't compare-and-set value in storage: {}",
              result.error().message());
    }
    return result.value();
  }

  outcome::result<common::Buffer> OffchainWorkerImpl::localStorageGet(
      StorageType storage_type, const common::Buffer &key) {
    auto &storage = getStorage(storage_type);
    auto result = storage.get(key);
    if (result.has_error()) {
      SL_WARN(log_, "Can't get value in storage: {}", result.error().message());
      return result.as_failure();
    }
    return std::move(result.value());
  }

  Result<RequestId, Failure> OffchainWorkerImpl::httpRequestStart(
      HttpMethod method, std::string_view uri, common::Buffer meta) {
    auto request_id = ++request_id_;

    auto request = std::make_shared<HttpRequest>(request_id);

    if (not request->init(method, uri, meta)) {
      return Failure();
    }

    auto is_emplaced = requests_.emplace(request_id, std::move(request)).second;

    if (is_emplaced) {
      return request_id;
    } else {
      return Failure();
    }
  }

  Result<Success, Failure> OffchainWorkerImpl::httpRequestAddHeader(
      RequestId id, std::string_view name, std::string_view value) {
    auto it = requests_.find(id);
    if (it == requests_.end()) {
      return Failure();
    }
    auto &request = it->second;

    request->addRequestHeader(name, value);

    return Success();
  }

  Result<Success, HttpError> OffchainWorkerImpl::httpRequestWriteBody(
      RequestId id,
      common::Buffer chunk,
      boost::optional<std::chrono::milliseconds> timeout) {
    auto it = requests_.find(id);
    if (it == requests_.end()) {
      return HttpError::InvalidId;
    }
    auto &request = it->second;

    auto result = request->writeRequestBody(chunk, timeout);

    return result;
  }

  std::vector<HttpStatus> OffchainWorkerImpl::httpResponseWait(
      const std::vector<RequestId> &ids,
      boost::optional<std::chrono::milliseconds> timeout) {
    std::vector<HttpStatus> result;
    result.reserve(ids.size());

    for (auto id : ids) {
      auto it = requests_.find(id);
      if (it == requests_.end()) {
        result.push_back(0);  // Invalid identifier
      }
      auto &request = it->second;

      result.push_back(request->status());
    }

    return result;
  }

  std::vector<std::pair<std::string, std::string>>
  OffchainWorkerImpl::httpResponseHeaders(RequestId id) {
    std::vector<std::pair<std::string, std::string>> result;
    auto it = requests_.find(id);
    if (it == requests_.end()) {
      return result;
    }
    auto &request = it->second;

    result = request->getResponseHeaders();

    return result;
  }

  Result<uint32_t, HttpError> OffchainWorkerImpl::httpResponseReadBody(
      RequestId id,
      common::Buffer &chunk,
      boost::optional<std::chrono::milliseconds> timeout) {
    auto it = requests_.find(id);
    if (it == requests_.end()) {
      return HttpError::InvalidId;
    }
    auto &request = it->second;

    auto result = request->readResponseBody(chunk, timeout);

    return result;
  }

  void OffchainWorkerImpl::setAuthorizedNodes(std::vector<libp2p::peer::PeerId>,
                                              bool authorized_only) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return;
  }

}  // namespace kagome::offchain
