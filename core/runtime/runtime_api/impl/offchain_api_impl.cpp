/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/offchain_api_impl.hpp"

namespace kagome::runtime {

  OffchainApiImpl::OffchainApiImpl() {}

  bool OffchainApiImpl::isValidator() const {
    return false;
  }

  common::Buffer OffchainApiImpl::submitTransaction(
      const primitives::Extrinsic &ext) {
    return {};
  }

  outcome::result<OpaqueNetworkState, Failure> OffchainApiImpl::networkState() {
    return Failure();
  }

  Timestamp OffchainApiImpl::offchainTimestamp() {
    return {};
  }

  void OffchainApiImpl::sleepUntil(Timestamp) {}

  RandomSeed OffchainApiImpl::randomSeed() {
    return {};
  }

  void OffchainApiImpl::localStorageSet(KindStorage kind,
                                        common::Buffer key,
                                        common::Buffer value) {
    return;
  }

  void OffchainApiImpl::localStorageClear(KindStorage kind,
                                          common::Buffer key) {
    return;
  }

  bool OffchainApiImpl::localStorageCompareAndSet(
      KindStorage kind,
      common::Buffer key,
      boost::optional<common::Buffer> expected,
      common::Buffer value) {
    return false;
  }

  common::Buffer OffchainApiImpl::localStorageGet(KindStorage kind,
                                                  common::Buffer key) {
    return {};
  }

  outcome::result<RequestId, Failure> OffchainApiImpl::httpRequestStart(
      Method method, common::Buffer uri, common::Buffer meta) {
    return Failure();
  }

  outcome::result<Success, Failure> OffchainApiImpl::httpRequestAddHeader(
      RequestId id, common::Buffer name, common::Buffer value) {
    return Success();
  }

  outcome::result<Success, HttpError> OffchainApiImpl::httpRequestWriteBody(
      RequestId id, common::Buffer chunk, boost::optional<Timestamp> deadline) {
    return Success();
  }

  outcome::result<Success, Failure> OffchainApiImpl::httpResponseWait(
      RequestId id, boost::optional<Timestamp> deadline) {
    return Failure();
  }

  std::vector<std::pair<std::string, std::string>>
  OffchainApiImpl::httpResponseHeaders(RequestId id) {
    return {};
  }

  outcome::result<uint32_t, HttpError> OffchainApiImpl::httpResponseReadBody(
      RequestId id,
      common::Buffer &chunk,
      boost::optional<Timestamp> deadline) {
    return outcome::success(0);
  }

  void OffchainApiImpl::setAuthorizedNodes(std::vector<libp2p::peer::PeerId>,
                                           bool authorized_only) {
    return;
  }

  void OffchainApiImpl::indexSet(common::Buffer key, common::Buffer value) {
    return;
  }

  void OffchainApiImpl::indexClear(common::Buffer key) {
    return;
  }

}  // namespace kagome::runtime
