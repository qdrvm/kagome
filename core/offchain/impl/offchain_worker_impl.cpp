/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "offchain/impl/offchain_worker_impl.hpp"

#include <thread>

#include <libp2p/host/host.hpp>

#include "api/service/author/author_api.hpp"
#include "application/app_configuration.hpp"
#include "crypto/hasher.hpp"
#include "offchain/impl/offchain_local_storage.hpp"
#include "runtime/executor.hpp"

namespace kagome::offchain {

  OffchainWorkerImpl::OffchainWorkerImpl(
      const application::AppConfiguration &app_config,
      std::shared_ptr<clock::SystemClock> clock,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<storage::BufferStorage> storage,
      std::shared_ptr<crypto::CSPRNG> random_generator,
      std::shared_ptr<api::AuthorApi> author_api,
      libp2p::Host &host,
      std::shared_ptr<OffchainPersistentStorage> persistent_storage,
      std::shared_ptr<runtime::Executor> executor,
      const primitives::BlockHeader &header)
      : app_config_(app_config),
        clock_(std::move(clock)),
        hasher_(std::move(hasher)),
        random_generator_(std::move(random_generator)),
        author_api_(std::move(author_api)),
        host_(host),
        persistent_storage_(std::move(persistent_storage)),
        executor_(std::move(executor)),
        header_(header),
        log_(log::createLogger(
            "OffchainWorker#" + std::to_string(header_.number), "offchain")) {
    BOOST_ASSERT(clock_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(storage);
    BOOST_ASSERT(random_generator_);
    BOOST_ASSERT(author_api_);
    BOOST_ASSERT(persistent_storage_);
    BOOST_ASSERT(executor_);

    auto hash = hasher_->blake2b_256(scale::encode(header_).value());
    const_cast<primitives::BlockInfo &>(block_) =
        primitives::BlockInfo(header_.number, hash);

    local_storage_ =
        std::make_shared<OffchainLocalStorageImpl>(std::move(storage));
  }

  outcome::result<void> OffchainWorkerImpl::run() {
    BOOST_ASSERT(not worker_of_this_thread().has_value());

    auto main_thread_func = [ocw = shared_from_this()] {
      soralog::util::setThreadName("ocw." + std::to_string(ocw->block_.number));

      worker_of_this_thread(*ocw);

      SL_TRACE(ocw->log_,
               "Offchain worker is started for block #{} hash={}",
               ocw->block_.number,
               ocw->block_.hash.toHex());

      auto res = ocw->executor_->callAt<void>(
          ocw->block_.hash, "OffchainWorkerApi_offchain_worker", ocw->header_);

      worker_of_this_thread(std::nullopt);

      if (res.has_failure()) {
        SL_ERROR(ocw->log_,
                 "Can't execute offchain worker for block #{} hash={}: {}",
                 ocw->block_.number,
                 ocw->block_.hash.toHex(),
                 res.error().message());
        return;
      }

      SL_DEBUG(ocw->log_,
               "Offchain worker is successfully executed for block #{} hash={}",
               ocw->block_.number,
               ocw->block_.hash.toHex());
    };

    try {
      std::thread(std::move(main_thread_func)).detach();
    } catch (const std::system_error &exception) {
      return outcome::failure(exception.code());
    } catch (...) {
      BOOST_UNREACHABLE_RETURN({});
    }

    return outcome::success();
  }

  bool OffchainWorkerImpl::isValidator() const {
    bool isValidator = app_config_.roles().flags.authority == 1;
    return isValidator;
  }

  Result<Success, Failure> OffchainWorkerImpl::submitTransaction(
      const primitives::Extrinsic &ext) {
    auto result = author_api_->submitExtrinsic(ext);
    if (result.has_value()) {
      return Success();
    }
    return Failure();
  }

  Result<OpaqueNetworkState, Failure> OffchainWorkerImpl::networkState() {
    auto peer_info = host_.getPeerInfo();
    OpaqueNetworkState result{.peer_id = peer_info.id};

    std::list<libp2p::multi::Multiaddress> address(peer_info.addresses.begin(),
                                                   peer_info.addresses.end());

    return result;
  }

  Timestamp OffchainWorkerImpl::timestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               clock_->now().time_since_epoch())
        .count();
  }

  void OffchainWorkerImpl::sleepUntil(Timestamp deadline) {
    auto ts = clock_->zero() + std::chrono::milliseconds(deadline);
    SL_TRACE(log_,
             "Falling asleep till {} (for {}ms)",
             deadline,
             std::chrono::duration_cast<std::chrono::milliseconds>(
                 ts - clock_->now())
                 .count());

    std::this_thread::sleep_until(decltype(clock_)::element_type::TimePoint(
        std::chrono::milliseconds(deadline)));
    SL_DEBUG(log_, "Woke up after sleeping");
  }

  RandomSeed OffchainWorkerImpl::randomSeed() {
    RandomSeed seed_bytes;
    random_generator_->fillRandomly(seed_bytes);
    return seed_bytes;
  }

  offchain::OffchainStorage &OffchainWorkerImpl::getStorage(
      StorageType storage_type) {
    switch (storage_type) {
      case StorageType::Persistent:
        return *persistent_storage_;

      case StorageType::Local:
        // TODO(xDimon):
        //  Need to implemented as soon as it will implemented in Substrate.
        //  Specification in not enough to implement it now.
        //  issue: https://github.com/soramitsu/kagome/issues/997
        SL_WARN(
            log_,
            "Attempt to use off-chain local storage which unavailable yet.");
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
      std::optional<std::reference_wrapper<const common::Buffer>> expected,
      common::Buffer value) {
    auto &storage = getStorage(storage_type);
    auto result = storage.compare_and_set(key, expected, std::move(value));
    if (result.has_error()) {
      SL_WARN(log_,
              "Can't compare-and-set value in storage: {}",
              result.error().message());
      return false;
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

    auto is_emplaced =
        active_http_requests_.emplace(request_id, std::move(request)).second;

    if (is_emplaced) {
      return request_id;
    } else {
      return Failure();
    }
  }

  Result<Success, Failure> OffchainWorkerImpl::httpRequestAddHeader(
      RequestId id, std::string_view name, std::string_view value) {
    auto it = active_http_requests_.find(id);
    if (it == active_http_requests_.end()) {
      return Failure();
    }
    auto &request = it->second;

    request->addRequestHeader(name, value);

    return Success();
  }

  Result<Success, HttpError> OffchainWorkerImpl::httpRequestWriteBody(
      RequestId id, common::Buffer chunk, std::optional<Timestamp> deadline) {
    std::optional<std::chrono::milliseconds> timeout = std::nullopt;
    if (deadline.has_value()) {
      timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::milliseconds(deadline.value())
          - clock_->now().time_since_epoch());
    }

    auto it = active_http_requests_.find(id);
    if (it == active_http_requests_.end()) {
      return HttpError::InvalidId;
    }
    auto &request = it->second;

    auto result = request->writeRequestBody(chunk, timeout);

    return result;
  }

  std::vector<HttpStatus> OffchainWorkerImpl::httpResponseWait(
      const std::vector<RequestId> &ids, std::optional<Timestamp> deadline) {
    std::vector<HttpStatus> result;
    result.reserve(ids.size());

    for (auto id : ids) {
      auto it = active_http_requests_.find(id);
      if (it == active_http_requests_.end()) {
        result.push_back(InvalidIdentifier);
      }
      auto &request = it->second;

      HttpStatus status;
      while ((status = request->status()) == 0) {
        if (deadline.has_value()
            and (clock_->zero() + std::chrono::milliseconds(deadline.value()))
                    < clock_->now()) {
          break;
        }
        std::this_thread::sleep_for(latency_of_waiting);
      }

      result.push_back(status ? status : DeadlineHasReached);
    }

    return result;
  }

  std::vector<std::pair<std::string, std::string>>
  OffchainWorkerImpl::httpResponseHeaders(RequestId id) {
    std::vector<std::pair<std::string, std::string>> result;
    auto it = active_http_requests_.find(id);
    if (it == active_http_requests_.end()) {
      return result;
    }
    auto &request = it->second;

    result = request->getResponseHeaders();

    return result;
  }

  Result<uint32_t, HttpError> OffchainWorkerImpl::httpResponseReadBody(
      RequestId id, common::Buffer &chunk, std::optional<Timestamp> deadline) {
    auto it = active_http_requests_.find(id);
    if (it == active_http_requests_.end()) {
      return HttpError::InvalidId;
    }
    auto &request = it->second;

    std::optional<std::chrono::milliseconds> timeout = std::nullopt;
    if (deadline.has_value()) {
      timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::milliseconds(deadline.value())
          - clock_->now().time_since_epoch());
    }

    auto result = request->readResponseBody(chunk, timeout);

    return result;
  }

  void OffchainWorkerImpl::setAuthorizedNodes(
      std::vector<libp2p::peer::PeerId> nodes, bool authorized_only) {
    // TODO(xDimon): Need to implement it
    //  issue: https://github.com/soramitsu/kagome/issues/998
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return;
  }

}  // namespace kagome::offchain
