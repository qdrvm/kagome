/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_OFFCHAINAPI
#define KAGOME_RUNTIME_OFFCHAINAPI

#include "runtime/runtime_api/offchain_worker.hpp"

#include <boost/optional.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "primitives/extrinsic.hpp"
#include "runtime/runtime_api/types.hpp"

namespace kagome::runtime {

  /**
   * @brief interface for Offchain runtime functions
   */
  class OffchainApi: public OffchainWorker {
   public:
    virtual ~OffchainApi() = default;

    // ---------------------- Manage of Off-Chain workers ----------------------

    virtual void spawnWorker(primitives::BlockInfo block_info) = 0;

    virtual void detachWorker() = 0;

    virtual void dropWorker() = 0;

    // ------------------------ Off-Chain Index methods ------------------------

    virtual void indexSet(common::Buffer key, common::Buffer value) = 0;

    virtual void indexClear(common::Buffer key) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_OFFCHAINAPI
