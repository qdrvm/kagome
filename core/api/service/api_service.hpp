/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_HPP
#define KAGOME_CORE_API_SERVICE_HPP

#include <functional>
#include <mutex>
#include <type_traits>
#include <unordered_map>

#include "api/jrpc/jrpc_server_impl.hpp"
#include "api/transport/listener.hpp"
#include "api/transport/rpc_thread_pool.hpp"
#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "containers/objects_cache.hpp"
#include "primitives/common.hpp"
#include "primitives/event_types.hpp"
#include "storage/trie/trie_storage.hpp"
#include "subscription/subscriber.hpp"

namespace kagome::api {
  template <typename T>
  using UCachedType = std::unique_ptr<T, void (*)(T *const)>;

  KAGOME_DECLARE_CACHE(
      api_service,
      KAGOME_CACHE_UNIT(std::string),
      KAGOME_CACHE_UNIT(std::vector<UCachedType<std::string>>));

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

    struct SessionExecutionContext {
      using AdditionMessageType =
          decltype(KAGOME_EXTRACT_UNIQUE_CACHE(api_service, std::string));
      using AdditionMessagesList = std::vector<AdditionMessageType>;
      using CachedAdditionMessagesList = decltype(
          KAGOME_EXTRACT_SHARED_CACHE(api_service, AdditionMessagesList));

      SubscribedSessionPtr storage_subscription;
      subscriptions::EventsSubscribedSessionPtr events_subscription;
      CachedAdditionMessagesList messages;
    };

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
        const std::vector<std::shared_ptr<JRpcProcessor>> &processors,
        SubscriptionEnginePtr subscription_engine,
        subscriptions::EventsSubscriptionEnginePtr events_engine,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<storage::trie::TrieStorage> trie_storage);

    virtual ~ApiService() = default;

    /** @see AppStateManager::takeControl */
    bool prepare();

    /** @see AppStateManager::takeControl */
    bool start();

    /** @see AppStateManager::takeControl */
    void stop();

    outcome::result<uint32_t> subscribeSessionToKeys(
        const std::vector<common::Buffer> &keys);

    outcome::result<bool> unsubscribeSessionFromIds(
        const std::vector<uint32_t> &subscription_id);

    outcome::result<uint32_t> subscribeFinalizedHeads();
    outcome::result<void> unsubscribeFinalizedHeads(uint32_t subscription_id);

    outcome::result<uint32_t> subscribeNewHeads();
    outcome::result<void> unsubscribeNewHeads(uint32_t subscription_id);

    outcome::result<uint32_t> subscribeRuntimeVersion();
    outcome::result<void> unsubscribeRuntimeVersion(uint32_t subscription_id);

   private:
    jsonrpc::Value createStateStorageEvent(const common::Buffer &key,
                                           const common::Buffer &value,
                                           const primitives::BlockHash &block);

    boost::optional<std::shared_ptr<SessionExecutionContext>> findSessionById(
        Session::SessionId id) {
      std::lock_guard guard(subscribed_sessions_cs_);
      if (auto it = subscribed_sessions_.find(id);
          subscribed_sessions_.end() != it)
        return it->second;

      return boost::none;
    }

    void removeSessionById(Session::SessionId id);
    std::shared_ptr<SessionExecutionContext> storeSessionWithId(
        Session::SessionId id, const std::shared_ptr<Session> &session);

    template <typename Func>
    auto for_session(kagome::api::Session::SessionId id, Func &&f) {
      if (auto session_context = findSessionById(id)) {
        BOOST_ASSERT(*session_context);
        return std::forward<Func>(f)(**session_context);
      }

      throw jsonrpc::InternalErrorFault(
          "Internal error. No session was stored for subscription.");
    }

    template <typename T>
    inline SessionExecutionContext::AdditionMessageType uploadFromCache(
        T &&value) {
      auto obj = KAGOME_EXTRACT_UNIQUE_CACHE(api_service, std::string);
      obj->assign(std::forward<T>(value));
      return obj;
    }

    inline SessionExecutionContext::CachedAdditionMessagesList
    uploadMessagesListFromCache() {
      auto obj = KAGOME_EXTRACT_UNIQUE_CACHE(
          api_service, SessionExecutionContext::AdditionMessagesList);
      obj->clear();
      return obj;
    }

   private:
    std::shared_ptr<api::RpcThreadPool> thread_pool_;
    std::vector<sptr<Listener>> listeners_;
    std::shared_ptr<JRpcServer> server_;
    common::Logger logger_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;

    std::mutex subscribed_sessions_cs_;
    std::unordered_map<Session::SessionId,
                       std::shared_ptr<SessionExecutionContext>>
        subscribed_sessions_;

    struct {
      SubscriptionEnginePtr storage;
      subscriptions::EventsSubscriptionEnginePtr events;
    } subscription_engines_;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_SERVICE_HPP
