/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_OFFCHAINWORKERIMPL
#define KAGOME_OFFCHAIN_OFFCHAINWORKERIMPL

#include "offchain/offchain_worker.hpp"

#include <boost/asio/io_context.hpp>

#include "crypto/random_generator.hpp"
#include "log/logger.hpp"
#include "network/types/own_peer_info.hpp"
#include "offchain/impl/http_request.hpp"
#include "offchain/impl/offchain_local_storage.hpp"
#include "offchain/impl/offchain_persistent_storage.hpp"
#include "primitives/block_header.hpp"

namespace kagome::api {
  class AuthorApi;
}
namespace kagome::application {
  class AppConfiguration;
}
namespace kagome::crypto {
  class Hasher;
}
namespace kagome::runtime {
  class Executor;
}
using namespace std::chrono_literals;

namespace kagome::offchain {
  class OffchainWorkerPool;

  class OffchainWorkerImpl final
      : public OffchainWorker,
        public std::enable_shared_from_this<OffchainWorkerImpl> {
   public:
    // Duration of sleeping in loop of waiting.
    // This value because all deadline quantized with milliseconds
    static constexpr auto latency_of_waiting = 1ms;

    OffchainWorkerImpl(
        const application::AppConfiguration &app_config,
        std::shared_ptr<clock::SystemClock> clock,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<storage::BufferStorage> storage,
        std::shared_ptr<crypto::CSPRNG> random_generator,
        std::shared_ptr<api::AuthorApi> author_api,
        const network::OwnPeerInfo &current_peer_info,
        std::shared_ptr<OffchainPersistentStorage> persistent_storage,
        std::shared_ptr<runtime::Executor> executor,
        const primitives::BlockHeader &header,
        std::shared_ptr<OffchainWorkerPool> ocw_pool);

    outcome::result<void> run() override;

    bool isValidator() const override;

    Result<Success, Failure> submitTransaction(
        const primitives::Extrinsic &ext) override;

    Result<OpaqueNetworkState, Failure> networkState() override;

    Timestamp timestamp() override;

    void sleepUntil(Timestamp timestamp) override;

    RandomSeed randomSeed() override;

    void localStorageSet(StorageType storage_type,
                         const common::BufferView &key,
                         common::Buffer value) override;

    void localStorageClear(StorageType storage_type,
                           const common::BufferView &key) override;

    bool localStorageCompareAndSet(StorageType storage_type,
                                   const common::BufferView &key,
                                   std::optional<common::BufferView> expected,
                                   common::Buffer value) override;

    outcome::result<common::Buffer> localStorageGet(
        StorageType storage_type, const common::BufferView &key) override;

    Result<RequestId, Failure> httpRequestStart(HttpMethod method,
                                                std::string_view uri,
                                                common::Buffer meta) override;

    Result<Success, Failure> httpRequestAddHeader(
        RequestId id, std::string_view name, std::string_view value) override;

    Result<Success, HttpError> httpRequestWriteBody(
        RequestId id,
        common::Buffer chunk,
        std::optional<Timestamp> deadline) override;

    std::vector<HttpStatus> httpResponseWait(
        const std::vector<RequestId> &ids,
        std::optional<Timestamp> deadline) override;

    std::vector<std::pair<std::string, std::string>> httpResponseHeaders(
        RequestId id) override;

    Result<uint32_t, HttpError> httpResponseReadBody(
        RequestId id,
        common::Buffer &chunk,
        std::optional<Timestamp> deadline) override;

    void setAuthorizedNodes(std::vector<libp2p::peer::PeerId> nodes,
                            bool authorized_only) override;

   private:
    OffchainStorage &getStorage(StorageType storage_type);

    const application::AppConfiguration &app_config_;
    std::shared_ptr<clock::SystemClock> clock_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::CSPRNG> random_generator_;
    std::shared_ptr<api::AuthorApi> author_api_;
    const network::OwnPeerInfo &current_peer_info_;
    std::shared_ptr<offchain::OffchainPersistentStorage> persistent_storage_;
    std::shared_ptr<offchain::OffchainLocalStorage> local_storage_;
    std::shared_ptr<runtime::Executor> executor_;
    const primitives::BlockHeader header_;
    const primitives::BlockInfo block_;
    std::shared_ptr<OffchainWorkerPool> ocw_pool_;
    log::Logger log_;

    int16_t request_id_ = 0;
    std::map<RequestId, std::shared_ptr<HttpRequest>> active_http_requests_;
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_OFFCHAINWORKERIMPL
