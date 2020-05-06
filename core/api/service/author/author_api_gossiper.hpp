/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_AUTHOR_AUTHOR_API_GOSSIPER_HPP
#define KAGOME_CORE_API_SERVICE_AUTHOR_AUTHOR_API_GOSSIPER_HPP

#include "network/types/transaction_announce.hpp"

namespace kagome::api {
  /**
   * Sends messages, related to author api, over the Gossip protocol
   */
  struct AuthorApiGossiper {
    virtual ~AuthorApiGossiper() = default;

    /**
     * Send TxAnnounce message
     * @param announce to be sent
     */
    virtual void transactionAnnounce(
        const network::TransactionAnnounce &announce) = 0;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_SERVICE_AUTHOR_AUTHOR_API_GOSSIPER_HPP
