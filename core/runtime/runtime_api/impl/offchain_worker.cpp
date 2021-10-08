/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/offchain_worker.hpp"

namespace kagome::runtime {

  OffchainWorkerImpl::OffchainWorkerImpl(primitives::BlockInfo block_info)
      : associated_block_(std::move(block_info)) {}

  void OffchainWorkerImpl::detach() {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return;
  }

  void OffchainWorkerImpl::drop() {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return;
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

  outcome::result<OpaqueNetworkState, Failure>
  OffchainWorkerImpl::networkState() {
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

  void OffchainWorkerImpl::localStorageSet(KindStorage kind,
                                           common::Buffer key,
                                           common::Buffer value) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return;
  }

  void OffchainWorkerImpl::localStorageClear(KindStorage kind,
                                             common::Buffer key) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return;
  }

  bool OffchainWorkerImpl::localStorageCompareAndSet(
      KindStorage kind,
      common::Buffer key,
      boost::optional<common::Buffer> expected,
      common::Buffer value) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return false;
  }

  common::Buffer OffchainWorkerImpl::localStorageGet(KindStorage kind,
                                                     common::Buffer key) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return {};
  }

  outcome::result<RequestId, Failure> OffchainWorkerImpl::httpRequestStart(
      Method method, common::Buffer uri, common::Buffer meta) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return Failure();
  }

  outcome::result<Success, Failure> OffchainWorkerImpl::httpRequestAddHeader(
      RequestId id, common::Buffer name, common::Buffer value) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return Success();
  }

  outcome::result<Success, HttpError> OffchainWorkerImpl::httpRequestWriteBody(
      RequestId id, common::Buffer chunk, boost::optional<Timestamp> deadline) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return Success();
  }

  outcome::result<Success, Failure> OffchainWorkerImpl::httpResponseWait(
      RequestId id, boost::optional<Timestamp> deadline) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return Failure();
  }

  std::vector<std::pair<std::string, std::string>>
  OffchainWorkerImpl::httpResponseHeaders(RequestId id) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return {};
  }

  outcome::result<uint32_t, HttpError> OffchainWorkerImpl::httpResponseReadBody(
      RequestId id,
      common::Buffer &chunk,
      boost::optional<Timestamp> deadline) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return outcome::success(0);
  }

  void OffchainWorkerImpl::setAuthorizedNodes(std::vector<libp2p::peer::PeerId>,
                                              bool authorized_only) {
    // TODO(xDimon): Need to implement it
    throw std::runtime_error(
        "This method of OffchainWorkerImpl is not implemented yet");
    return;
  }

}  // namespace kagome::runtime
