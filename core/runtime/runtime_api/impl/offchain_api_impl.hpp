/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_OFFCHAINAPIIMPL
#define KAGOME_RUNTIME_OFFCHAINAPIIMPL

#include "runtime/runtime_api/offchain_api.hpp"

#include "runtime/runtime_api/impl/offchain_worker.hpp"

namespace kagome::runtime {

  class OffchainApiImpl final : public OffchainApi {
   public:
    OffchainApiImpl();

    // ---------------------- Manage of Off-Chain workers ----------------------

    void spawnWorker(primitives::BlockInfo block_info) override;

    void detachWorker() override;

    void dropWorker() override;

    // ------------------------- Off-Chain API methods -------------------------

    bool isValidator() const override;

    common::Buffer submitTransaction(const primitives::Extrinsic &ext) override;

    outcome::result<OpaqueNetworkState, Failure> networkState() override;

    Timestamp offchainTimestamp() override;

    void sleepUntil(Timestamp) override;

    RandomSeed randomSeed() override;

    void localStorageSet(KindStorage kind,
                         common::Buffer key,
                         common::Buffer value) override;

    void localStorageClear(KindStorage kind, common::Buffer key) override;

    bool localStorageCompareAndSet(KindStorage kind,
                                   common::Buffer key,
                                   boost::optional<common::Buffer> expected,
                                   common::Buffer value) override;

    common::Buffer localStorageGet(KindStorage kind,
                                   common::Buffer key) override;

    outcome::result<RequestId, Failure> httpRequestStart(
        Method method, common::Buffer uri, common::Buffer meta) override;

    outcome::result<Success, Failure> httpRequestAddHeader(
        RequestId id, common::Buffer name, common::Buffer value) override;

    outcome::result<Success, HttpError> httpRequestWriteBody(
        RequestId id,
        common::Buffer chunk,
        boost::optional<Timestamp> deadline) override;

    outcome::result<Success, Failure> httpResponseWait(
        RequestId id, boost::optional<Timestamp> deadline) override;

    std::vector<std::pair<std::string, std::string>> httpResponseHeaders(
        RequestId id) override;

    outcome::result<uint32_t, HttpError> httpResponseReadBody(
        RequestId id,
        common::Buffer &chunk,
        boost::optional<Timestamp> deadline) override;

    void setAuthorizedNodes(std::vector<libp2p::peer::PeerId>,
                            bool authorized_only) override;

    // ------------------------ Off-Chain Index methods ------------------------

    void indexSet(common::Buffer key, common::Buffer value) override;

    void indexClear(common::Buffer key) override;

   private:
    boost::optional<std::shared_ptr<OffchainWorkerImpl>> worker_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_OFFCHAINAPIIMPL
