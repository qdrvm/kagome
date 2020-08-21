/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_CATCHUPOBSERVER
#define KAGOME_CORE_CONSENSUS_GRANDPA_CATCHUPOBSERVER

#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /**
   * @class CatchUpObserver
   * @brief observes incoming catch-up messages. Abstraction of a network.
   */
  struct CatchUpObserver {
    virtual ~CatchUpObserver() = default;

    /**
     * Handler of grandpa catch-up-request messages
     * @param msg vote message
     */
    virtual void onCatchUpRequest(const CatchUpRequest &msg) = 0;

    /**
     * Handler of grandpa catch-up-response messages
     * @param msg vote message
     */
    virtual void onCatchUpResponse(const CatchUpResponse &msg) = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_CATCHUPOBSERVER
