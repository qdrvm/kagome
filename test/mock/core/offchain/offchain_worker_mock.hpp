/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_WORKER_MOCK_HPP
#define KAGOME_OFFCHAIN_WORKER_MOCK_HPP

#include <gmock/gmock.h>

#include "offchain/offchain_worker.hpp"

namespace kagome::offchain {

  class OffchainWorkerMock : public OffchainWorker {
   public:
    MOCK_METHOD(outcome::result<void>, run, (), (override));

    MOCK_METHOD(bool, isValidator, (), (const, override));

    MOCK_METHOD((Result<Success, Failure>),
                submitTransaction,
                (const primitives::Extrinsic &ext),
                (override));

    MOCK_METHOD((Result<OpaqueNetworkState, Failure>),
                networkState,
                (),
                (override));

    MOCK_METHOD(Timestamp, timestamp, (), (override));

    MOCK_METHOD(void, sleepUntil, (Timestamp), (override));

    MOCK_METHOD(RandomSeed, randomSeed, (), (override));

    MOCK_METHOD(void,
                localStorageSet,
                (StorageType storage_type,
                 const common::BufferView &key,
                 common::Buffer value),
                (override));

    MOCK_METHOD(void,
                localStorageClear,
                (StorageType storage_type, const common::BufferView &key),
                (override));

    MOCK_METHOD(
        bool,
        localStorageCompareAndSet,
        (StorageType storage_type,
         const common::BufferView &key,
         std::optional<common::BufferView> expected,
         common::Buffer value),
        (override));

    MOCK_METHOD(outcome::result<common::Buffer>,
                localStorageGet,
                (StorageType storage_type, const common::BufferView &key),
                (override));

    MOCK_METHOD((Result<RequestId, Failure>),
                httpRequestStart,
                (HttpMethod method, std::string_view uri, common::Buffer meta),
                (override));

    MOCK_METHOD((Result<Success, Failure>),
                httpRequestAddHeader,
                (RequestId id, std::string_view name, std::string_view value),
                (override));

    MOCK_METHOD((Result<Success, HttpError>),
                httpRequestWriteBody,
                (RequestId id,
                 common::Buffer chunk,
                 std::optional<Timestamp> deadline),
                (override));

    MOCK_METHOD(std::vector<HttpStatus>,
                httpResponseWait,
                (const std::vector<RequestId> &ids,
                 std::optional<Timestamp> deadline),
                (override));

    MOCK_METHOD((std::vector<std::pair<std::string, std::string>>),
                httpResponseHeaders,
                (RequestId id),
                (override));

    MOCK_METHOD((Result<uint32_t, HttpError>),
                httpResponseReadBody,
                (RequestId id,
                 common::Buffer &chunk,
                 std::optional<Timestamp> deadline),
                (override));

    MOCK_METHOD(void,
                setAuthorizedNodes,
                (std::vector<libp2p::peer::PeerId> nodes, bool authorized_only),
                (override));
  };

}  // namespace kagome::offchain

#endif /* KAGOME_OFFCHAIN_WORKER_MOCK_HPP */
