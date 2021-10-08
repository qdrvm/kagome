/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/offchain_api_impl.hpp"

#include "runtime/runtime_api/impl/offchain_worker.hpp"

namespace kagome::runtime {

  OffchainApiImpl::OffchainApiImpl() {}

  void OffchainApiImpl::spawnWorker(primitives::BlockInfo block_info) {
    BOOST_ASSERT(not worker_.has_value());
    worker_.emplace(
        std::make_shared<OffchainWorkerImpl>(std::move(block_info)));
  }

  void OffchainApiImpl::detachWorker() {
    if (worker_.has_value()) {
      auto &worker = **worker_;

      worker.detach();
      worker_.reset();
    }
  }

  void OffchainApiImpl::dropWorker() {
    if (worker_.has_value()) {
      auto &worker = **worker_;

      worker.drop();
      worker_.reset();
    }
  }

  bool OffchainApiImpl::isValidator() const {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.isValidator();
  }

  common::Buffer OffchainApiImpl::submitTransaction(
      const primitives::Extrinsic &ext) {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.submitTransaction(ext);
  }

  outcome::result<OpaqueNetworkState, Failure> OffchainApiImpl::networkState() {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.networkState();
  }

  Timestamp OffchainApiImpl::offchainTimestamp() {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.offchainTimestamp();
  }

  void OffchainApiImpl::sleepUntil(Timestamp timestamp) {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.sleepUntil(timestamp);
  }

  RandomSeed OffchainApiImpl::randomSeed() {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.randomSeed();
  }

  void OffchainApiImpl::localStorageSet(KindStorage kind,
                                        common::Buffer key,
                                        common::Buffer value) {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.localStorageSet(kind, std::move(key), std::move(value));
  }

  void OffchainApiImpl::localStorageClear(KindStorage kind,
                                          common::Buffer key) {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.localStorageClear(kind, std::move(key));
  }

  bool OffchainApiImpl::localStorageCompareAndSet(
      KindStorage kind,
      common::Buffer key,
      boost::optional<common::Buffer> expected,
      common::Buffer value) {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.localStorageCompareAndSet(
        kind, std::move(key), std::move(expected), std::move(value));
  }

  common::Buffer OffchainApiImpl::localStorageGet(KindStorage kind,
                                                  common::Buffer key) {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.localStorageGet(kind, std::move(key));
  }

  outcome::result<RequestId, Failure> OffchainApiImpl::httpRequestStart(
      Method method, common::Buffer uri, common::Buffer meta) {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.httpRequestStart(method, std::move(uri), std::move(meta));
  }

  outcome::result<Success, Failure> OffchainApiImpl::httpRequestAddHeader(
      RequestId id, common::Buffer name, common::Buffer value) {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.httpRequestAddHeader(id, std::move(name), std::move(value));
  }

  outcome::result<Success, HttpError> OffchainApiImpl::httpRequestWriteBody(
      RequestId id, common::Buffer chunk, boost::optional<Timestamp> deadline) {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.httpRequestWriteBody(
        id, std::move(chunk), std::move(deadline));
  }

  outcome::result<Success, Failure> OffchainApiImpl::httpResponseWait(
      RequestId id, boost::optional<Timestamp> deadline) {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.httpResponseWait(id, std::move(deadline));
  }

  std::vector<std::pair<std::string, std::string>>
  OffchainApiImpl::httpResponseHeaders(RequestId id) {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.httpResponseHeaders(id);
  }

  outcome::result<uint32_t, HttpError> OffchainApiImpl::httpResponseReadBody(
      RequestId id,
      common::Buffer &chunk,
      boost::optional<Timestamp> deadline) {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.httpResponseReadBody(id, chunk, std::move(deadline));
  }

  void OffchainApiImpl::setAuthorizedNodes(
      std::vector<libp2p::peer::PeerId> nodes, bool authorized_only) {
    BOOST_ASSERT(worker_.has_value());
    auto &worker = **worker_;

    return worker.setAuthorizedNodes(std::move(nodes), authorized_only);
  }

  void OffchainApiImpl::indexSet(common::Buffer key, common::Buffer value) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainApiImpl is not implemented yet");
    return;
  }

  void OffchainApiImpl::indexClear(common::Buffer key) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainApiImpl is not implemented yet");
    return;
  }

}  // namespace kagome::runtime
