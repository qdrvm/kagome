/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_OFFCHAINWORKERIMPL
#define KAGOME_OFFCHAIN_OFFCHAINWORKERIMPL

#include "offchain/offchain_worker.hpp"

#include <boost/asio/io_context.hpp>

#include "offchain/impl/http_request.hpp"
#include "primitives/block_header.hpp"

namespace kagome::runtime {
  class Executor;
}

namespace kagome::offchain {

  class OffchainWorkerImpl final
      : public OffchainWorker,
        public std::enable_shared_from_this<OffchainWorkerImpl> {
   public:
    OffchainWorkerImpl(std::shared_ptr<runtime::Executor> executor,
                       const primitives::BlockHash &block,
                       const primitives::BlockHeader &header);

    static outcome::result<void> run(
        std::shared_ptr<OffchainWorkerImpl> worker);

    bool isValidator() const override;

    common::Buffer submitTransaction(const primitives::Extrinsic &ext) override;

    Result<OpaqueNetworkState, Failure> networkState() override;

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

    Result<RequestId, Failure> httpRequestStart(Method method,
                                                std::string_view uri,
                                                common::Buffer meta) override;

    Result<Success, Failure> httpRequestAddHeader(
        RequestId id, std::string_view name, std::string_view value) override;

    Result<Success, HttpError> httpRequestWriteBody(
        RequestId id,
        common::Buffer chunk,
        boost::optional<Timestamp> deadline) override;

    std::vector<HttpStatus> httpResponseWait(
        const std::vector<RequestId> &ids,
        boost::optional<Timestamp> deadline) override;

    std::vector<std::pair<std::string, std::string>> httpResponseHeaders(
        RequestId id) override;

    Result<uint32_t, HttpError> httpResponseReadBody(
        RequestId id,
        common::Buffer &chunk,
        boost::optional<Timestamp> deadline) override;

    void setAuthorizedNodes(std::vector<libp2p::peer::PeerId>,
                            bool authorized_only) override;

   private:
    std::shared_ptr<runtime::Executor> executor_;
    const primitives::BlockInfo associated_block_;
    const primitives::BlockHeader header_;

    boost::asio::io_context io_context_;
    int16_t request_id_ = 0;
    std::map<RequestId, std::shared_ptr<HttpRequest>> requests_;
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_OFFCHAINWORKERIMPL
