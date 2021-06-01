/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_SERVICE_IMPL_HPP
#define KAGOME_API_SERVICE_IMPL_HPP

#include "api/service/api_service.hpp"

#include <functional>
#include <mutex>
#include <type_traits>
#include <unordered_map>

#include <jsonrpc-lean/fault.h>

#include "api/transport/rpc_thread_pool.hpp"
#include "api/transport/session.hpp"
#include "common/buffer.hpp"
#include "containers/objects_cache.hpp"
#include "log/logger.hpp"
#include "primitives/block_id.hpp"
#include "primitives/event_types.hpp"
#include "subscription/subscription_engine.hpp"

namespace kagome::api {
  class JRpcProcessor;
  class JRpcServer;
  class Listener;
}  // namespace kagome::api
namespace kagome::application {
  class AppStateManager;
}
namespace kagome::blockchain {
  class BlockTree;
}
namespace kagome::primitives {
  struct Transaction;
}
namespace kagome::storage::trie {
  class TrieStorage;
}
namespace kagome::subscription {
  class ExtrinsicEventKeyRepository;
}

namespace jsonrpc {
  class Value;
}

namespace kagome::api {

  template <typename T>
  using UCachedType = std::unique_ptr<T, void (*)(T *const)>;

  KAGOME_DECLARE_CACHE(api_service,
                       KAGOME_CACHE_UNIT(std::string),
                       KAGOME_CACHE_UNIT(std::vector<UCachedType<std::string>>))

  class JRpcProcessor;

  /**
   * Service listening for incoming JSON RPC request
   */
  class ApiServiceImpl final
      : public ApiService,
        public std::enable_shared_from_this<ApiServiceImpl> {
    using ChainEventSubscriberPtr = primitives::events::ChainEventSubscriberPtr;
    using StorageEventSubscriberPtr =
        primitives::events::StorageEventSubscriberPtr;
    using ExtrinsicEventSubscriberPtr =
        primitives::events::ExtrinsicEventSubscriberPtr;
    using ChainSubscriptionEnginePtr =
        primitives::events::ChainSubscriptionEnginePtr;
    using StorageSubscriptionEnginePtr =
        primitives::events::StorageSubscriptionEnginePtr;
    using ExtrinsicSubscriptionEnginePtr =
        primitives::events::ExtrinsicSubscriptionEnginePtr;
    using ChainEventSubscriber = primitives::events::ChainEventSubscriber;
    using ExtrinsicEventSubscriber =
        primitives::events::ExtrinsicEventSubscriber;
    using StorageEventSubscriber = primitives::events::StorageEventSubscriber;
    using ExtrinsicSubscriptionEngine =
        primitives::events::ExtrinsicSubscriptionEngine;

    using SessionPtr = std::shared_ptr<Session>;

    /// subscription set id from subscription::SubscriptionEngine
    using SubscriptionSetId = subscription::SubscriptionSetId;

    /// subscription id for pubsub API methods
    using PubsubSubscriptionId = uint32_t;

    using Buffer = common::Buffer;

    struct SessionSubscriptions {
      using AdditionMessageType =
          decltype(KAGOME_EXTRACT_UNIQUE_CACHE(api_service, std::string));
      using AdditionMessagesList = std::vector<AdditionMessageType>;
      using CachedAdditionMessagesList = decltype(
          KAGOME_EXTRACT_SHARED_CACHE(api_service, AdditionMessagesList));

      StorageEventSubscriberPtr storage_sub;
      ChainEventSubscriberPtr chain_sub;
      ExtrinsicEventSubscriberPtr ext_sub;
      CachedAdditionMessagesList messages;
    };

   public:
    template <class T>
    using sptr = std::shared_ptr<T>;

    struct ListenerList {
      std::vector<sptr<Listener>> listeners;
    };
    struct ProcessorSpan {
      gsl::span<sptr<JRpcProcessor>> processors;
    };

    /**
     * @brief constructor
     * @param context - reference to the io context
     * @param listener - a shared ptr to the endpoint listener instance
     * @param processors - shared ptrs to JSON processor instances
     */
    ApiServiceImpl(
        const std::shared_ptr<application::AppStateManager> &app_state_manager,
        std::shared_ptr<api::RpcThreadPool> thread_pool,
        ListenerList listeners,
        std::shared_ptr<JRpcServer> server,
        const ProcessorSpan &processors,
        StorageSubscriptionEnginePtr storage_sub_engine,
        ChainSubscriptionEnginePtr chain_sub_engine,
        ExtrinsicSubscriptionEnginePtr ext_sub_engine,
        std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
            extrinsic_event_key_repo,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<storage::trie::TrieStorage> trie_storage);

    ~ApiServiceImpl() override = default;

    /** @see AppStateManager::takeControl */
    bool prepare() override;

    /** @see AppStateManager::takeControl */
    bool start() override;

    /** @see AppStateManager::takeControl */
    void stop() override;

    outcome::result<uint32_t> subscribeSessionToKeys(
        const std::vector<common::Buffer> &keys) override;

    outcome::result<bool> unsubscribeSessionFromIds(
        const std::vector<PubsubSubscriptionId> &subscription_id) override;

    outcome::result<PubsubSubscriptionId> subscribeFinalizedHeads() override;
    outcome::result<bool> unsubscribeFinalizedHeads(
        PubsubSubscriptionId subscription_id) override;

    outcome::result<PubsubSubscriptionId> subscribeNewHeads() override;
    outcome::result<bool> unsubscribeNewHeads(
        PubsubSubscriptionId subscription_id) override;

    outcome::result<PubsubSubscriptionId> subscribeRuntimeVersion() override;
    outcome::result<bool> unsubscribeRuntimeVersion(
        PubsubSubscriptionId subscription_id) override;

    outcome::result<PubsubSubscriptionId> subscribeForExtrinsicLifecycle(
        const primitives::Transaction &tx) override;
    outcome::result<bool> unsubscribeFromExtrinsicLifecycle(
        PubsubSubscriptionId subscription_id) override;

   private:
    jsonrpc::Value createStateStorageEvent(const common::Buffer &key,
                                           const common::Buffer &value,
                                           const primitives::BlockHash &block);

    boost::optional<std::shared_ptr<SessionSubscriptions>> findSessionById(
        Session::SessionId id) {
      std::lock_guard guard(subscribed_sessions_cs_);
      if (auto it = subscribed_sessions_.find(id);
          subscribed_sessions_.end() != it)
        return it->second;

      return boost::none;
    }
    void removeSessionById(Session::SessionId id);
    std::shared_ptr<SessionSubscriptions> storeSessionWithId(
        Session::SessionId id, const std::shared_ptr<Session> &session);

    void onSessionRequest(std::string_view request,
                          std::shared_ptr<Session> session);
    void onSessionClose(Session::SessionId id, SessionType);
    void onStorageEvent(SubscriptionSetId set_id,
                        SessionPtr &session,
                        const Buffer &key,
                        const Buffer &data,
                        const common::Hash256 &block);
    void onChainEvent(SubscriptionSetId set_id,
                      SessionPtr &session,
                      primitives::events::ChainEventType event_type,
                      const primitives::events::ChainEventParams &params);
    void onExtrinsicEvent(
        SubscriptionSetId set_id,
        SessionPtr &session,
        primitives::events::SubscribedExtrinsicId id,
        const primitives::events::ExtrinsicLifecycleEvent &params);

    template <typename Func>
    auto withSession(kagome::api::Session::SessionId id, Func &&f) {
      if (auto session_context = findSessionById(id)) {
        BOOST_ASSERT(*session_context);
        return std::forward<Func>(f)(**session_context);
      }

      throw jsonrpc::InternalErrorFault(
          "Internal error. No session was stored for subscription.");
    }

    template <typename T>
    inline SessionSubscriptions::AdditionMessageType uploadFromCache(
        T &&value) {
      auto obj = KAGOME_EXTRACT_UNIQUE_CACHE(api_service, std::string);
      obj->assign(std::forward<T>(value));
      return obj;
    }

    inline SessionSubscriptions::CachedAdditionMessagesList
    uploadMessagesListFromCache() {
      auto obj = KAGOME_EXTRACT_UNIQUE_CACHE(
          api_service, SessionSubscriptions::AdditionMessagesList);
      obj->clear();
      return obj;
    }

    std::shared_ptr<api::RpcThreadPool> thread_pool_;
    std::vector<sptr<Listener>> listeners_;
    std::shared_ptr<JRpcServer> server_;
    log::Logger logger_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<storage::trie::TrieStorage> trie_storage_;

    std::mutex subscribed_sessions_cs_;
    std::unordered_map<Session::SessionId,
                       std::shared_ptr<SessionSubscriptions>>
        subscribed_sessions_;

    struct {
      StorageSubscriptionEnginePtr storage;
      ChainSubscriptionEnginePtr chain;
      ExtrinsicSubscriptionEnginePtr ext;
    } subscription_engines_;
    std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
        extrinsic_event_key_repo_;
  };
}  // namespace kagome::api

#endif  // KAGOME_API_SERVICE_IMPL_HPP
