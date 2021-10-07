/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_OFFCHAINAPI
#define KAGOME_RUNTIME_OFFCHAINAPI

#include <boost/optional.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/extrinsic.hpp"
#include "runtime/runtime_api/types.hpp"

namespace kagome::runtime {

  /**
   * @brief interface for Offchain runtime functions
   */
  class OffchainApi {
   public:
    virtual ~OffchainApi() = default;

    virtual bool isValidator() const = 0;

    virtual common::Buffer submitTransaction(
        const primitives::Extrinsic &ext) = 0;

    virtual outcome::result<OpaqueNetworkState, Failure> networkState() = 0;

    virtual Timestamp offchainTimestamp() = 0;

    virtual void sleepUntil(Timestamp) = 0;

    virtual RandomSeed randomSeed() = 0;

    virtual void localStorageSet(KindStorage kind,
                                 common::Buffer key,
                                 common::Buffer value) = 0;

    virtual void localStorageClear(KindStorage kind, common::Buffer key) = 0;

    virtual bool localStorageCompareAndSet(
        KindStorage kind,
        common::Buffer key,
        boost::optional<common::Buffer> expected,
        common::Buffer value) = 0;

    virtual common::Buffer localStorageGet(KindStorage kind,
                                           common::Buffer key) = 0;

    virtual outcome::result<RequestId, Failure> httpRequestStart(
        Method method, common::Buffer uri, common::Buffer meta) = 0;

    virtual outcome::result<Success, Failure> httpRequestAddHeader(
        RequestId id, common::Buffer name, common::Buffer value) = 0;

    virtual outcome::result<Success, HttpError> httpRequestWriteBody(
        RequestId id,
        common::Buffer chunk,
        boost::optional<Timestamp> deadline) = 0;

    virtual outcome::result<Success, Failure> httpResponseWait(
        RequestId id, boost::optional<Timestamp> deadline) = 0;

    virtual std::vector<std::pair<std::string, std::string>>
    httpResponseHeaders(RequestId id) = 0;

    virtual outcome::result<uint32_t, HttpError> httpResponseReadBody(
        RequestId id,
        common::Buffer &chunk,
        boost::optional<Timestamp> deadline) = 0;

    virtual void setAuthorizedNodes(std::vector<libp2p::peer::PeerId>,
                                    bool authorized_only) = 0;

    virtual void indexSet(common::Buffer key, common::Buffer value) = 0;

    virtual void indexClear(common::Buffer key) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_OFFCHAINAPI
