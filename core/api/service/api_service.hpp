/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_HPP
#define KAGOME_CORE_API_SERVICE_HPP

#include <functional>
#include <gsl/span>
#include <mutex>
#include <unordered_map>

#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/transport/listener.hpp"
#include "api/transport/rpc_thread_pool.hpp"
#include "application/app_state_manager.hpp"
#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "primitives/common.hpp"
#include "subscription/subscriber.hpp"

namespace kagome::api {

  class JRpcProcessor;

  /**
   * Service listening for incoming JSON RPC request
   */
  class ApiService final : public std::enable_shared_from_this<ApiService> {
    using SessionPtr = std::shared_ptr<Session>;

    using SubscribedSessionType =
        subscription::Subscriber<common::Buffer,
                                 SessionPtr,
                                 common::Buffer,
                                 primitives::BlockHash>;
    using SubscribedSessionPtr = std::shared_ptr<SubscribedSessionType>;

    using SubscriptionEngineType =
        subscription::SubscriptionEngine<common::Buffer,
                                         SessionPtr,
                                         common::Buffer,
                                         primitives::BlockHash>;
    using SubscriptionEnginePtr = std::shared_ptr<SubscriptionEngineType>;

   public:
    template <class T>
    using sptr = std::shared_ptr<T>;

    /**
     * @brief constructor
     * @param context - reference to the io context
     * @param listener - a shared ptr to the endpoint listener instance
     * @param processors - shared ptrs to JSON processor instances
     */
    ApiService(
        const std::shared_ptr<application::AppStateManager> &app_state_manager,
        std::shared_ptr<api::RpcThreadPool> thread_pool,
        std::vector<std::shared_ptr<Listener>> listeners,
        std::shared_ptr<JRpcServer> server,
        gsl::span<std::shared_ptr<JRpcProcessor>> processors,
        SubscriptionEnginePtr subscription_engine);

    virtual ~ApiService() = default;

    void prepare();
    void start();
    void stop();

    outcome::result<uint32_t> subscribeSessionToKeys(
        const std::vector<common::Buffer> &keys);

    outcome::result<void> unsubscribeSessionFromIds(
        const std::vector<uint32_t> &subscription_id);

   private:
    SubscribedSessionPtr findSessionById(Session::SessionId id);
    void removeSessionById(Session::SessionId id);
    SubscribedSessionPtr storeSessionWithId(
        Session::SessionId id, const std::shared_ptr<Session> &session);

   private:
    std::shared_ptr<api::RpcThreadPool> thread_pool_;
    std::vector<sptr<Listener>> listeners_;
    std::shared_ptr<JRpcServer> server_;
    common::Logger logger_;

    std::mutex subscribed_sessions_cs_;
    std::unordered_map<Session::SessionId, SubscribedSessionPtr>
        subscribed_sessions_;
    SubscriptionEnginePtr subscription_engine_;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_SERVICE_HPP
