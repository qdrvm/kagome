/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_HPP
#define KAGOME_CORE_API_SERVICE_HPP

#include <vector>

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/transaction.hpp"

namespace kagome::api {

  /**
   * Service listening for incoming JSON RPC request
   */
  class ApiService {
   public:
    /// subscription id for pubsub API methods
    using PubsubSubscriptionId = uint32_t;

    template <class T>
    using sptr = std::shared_ptr<T>;

    virtual ~ApiService() = default;

    /** @see AppStateManager::takeControl */
    virtual bool prepare() = 0;

    /** @see AppStateManager::takeControl */
    virtual bool start() = 0;

    /** @see AppStateManager::takeControl */
    virtual void stop() = 0;

    virtual outcome::result<uint32_t> subscribeSessionToKeys(
        const std::vector<common::Buffer> &keys) = 0;

    virtual outcome::result<bool> unsubscribeSessionFromIds(
        const std::vector<PubsubSubscriptionId> &subscription_id) = 0;

    virtual outcome::result<PubsubSubscriptionId> subscribeFinalizedHeads() = 0;
    virtual outcome::result<bool> unsubscribeFinalizedHeads(
        PubsubSubscriptionId subscription_id) = 0;

    virtual outcome::result<PubsubSubscriptionId> subscribeNewHeads() = 0;
    virtual outcome::result<bool> unsubscribeNewHeads(
        PubsubSubscriptionId subscription_id) = 0;

    virtual outcome::result<PubsubSubscriptionId> subscribeRuntimeVersion() = 0;
    virtual outcome::result<bool> unsubscribeRuntimeVersion(
        PubsubSubscriptionId subscription_id) = 0;

    virtual outcome::result<PubsubSubscriptionId>
    subscribeForExtrinsicLifecycle(
        const primitives::Transaction::Hash &tx_hash) = 0;

    virtual outcome::result<bool> unsubscribeFromExtrinsicLifecycle(
        PubsubSubscriptionId subscription_id) = 0;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_SERVICE_HPP
