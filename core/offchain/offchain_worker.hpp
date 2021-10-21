/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OFFCHAIN_OFFCHAINWORKER
#define KAGOME_OFFCHAIN_OFFCHAINWORKER

#include <boost/optional.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "common/buffer.hpp"
#include "offchain/types.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::offchain {

  class OffchainWorker {
   public:
    static const boost::optional<OffchainWorker &> &current() {
      return current_opt();
    }

    virtual ~OffchainWorker() = default;

    virtual outcome::result<void> run() = 0;

    // ------------------------- Off-Chain API methods -------------------------

    virtual bool isValidator() const = 0;

    virtual common::Buffer submitTransaction(
        const primitives::Extrinsic &ext) = 0;

    virtual Result<OpaqueNetworkState, Failure> networkState() = 0;

    virtual Timestamp timestamp() = 0;

    virtual void sleepUntil(Timestamp) = 0;

    virtual RandomSeed randomSeed() = 0;

    virtual void localStorageSet(StorageType storage_type,
                                 const common::Buffer &key,
                                 common::Buffer value) = 0;

    virtual void localStorageClear(StorageType storage_type,
                                   const common::Buffer &key) = 0;

    virtual bool localStorageCompareAndSet(
        StorageType storage_type,
        const common::Buffer &key,
        boost::optional<const common::Buffer &> expected,
        common::Buffer value) = 0;

    virtual outcome::result<common::Buffer> localStorageGet(
        StorageType storage_type, const common::Buffer &key) = 0;

    virtual Result<RequestId, Failure> httpRequestStart(
        HttpMethod method, std::string_view uri, common::Buffer meta) = 0;

    virtual Result<Success, Failure> httpRequestAddHeader(
        RequestId id, std::string_view name, std::string_view value) = 0;

    virtual Result<Success, HttpError> httpRequestWriteBody(
        RequestId id,
        common::Buffer chunk,
        boost::optional<Timestamp> deadline) = 0;

    virtual std::vector<HttpStatus> httpResponseWait(
        const std::vector<RequestId> &ids,
        boost::optional<Timestamp> deadline) = 0;

    virtual std::vector<std::pair<std::string, std::string>>
    httpResponseHeaders(RequestId id) = 0;

    virtual Result<uint32_t, HttpError> httpResponseReadBody(
        RequestId id,
        common::Buffer &chunk,
        boost::optional<Timestamp> deadline) = 0;

    virtual void setAuthorizedNodes(std::vector<libp2p::peer::PeerId>,
                                    bool authorized_only) = 0;

   protected:
    static void current(boost::optional<OffchainWorker &> worker) {
      current_opt() = std::move(worker);
    }

   private:
    static boost::optional<OffchainWorker &> &current_opt() {
      static thread_local boost::optional<OffchainWorker &> current_opt =
          boost::none;
      return current_opt;
    }
  };

}  // namespace kagome::offchain

#endif  // KAGOME_OFFCHAIN_OFFCHAINWORKER
