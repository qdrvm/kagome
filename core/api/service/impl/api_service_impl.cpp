/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/impl/api_service_impl.hpp"

#include <boost/algorithm/string/replace.hpp>

#include "api/jrpc/jrpc_processor.hpp"
#include "api/jrpc/jrpc_server.hpp"
#include "api/jrpc/value_converter.hpp"
#include "api/transport/listener.hpp"
#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/hexutil.hpp"
#include "common/no_fn.hpp"
#include "primitives/common.hpp"
#include "primitives/transaction.hpp"
#include "runtime/runtime_api/core.hpp"
#include "storage/trie/trie_storage.hpp"
#include "subscription/extrinsic_event_key_repository.hpp"
#include "subscription/subscriber.hpp"

namespace {
  thread_local class {
   private:
    std::optional<kagome::api::Session::SessionId> bound_session_id_ =
        std::nullopt;

   public:
    void storeSessionId(kagome::api::Session::SessionId id) {
      bound_session_id_ = id;
    }
    void releaseSessionId() {
      bound_session_id_ = std::nullopt;
    }
    std::optional<kagome::api::Session::SessionId> fetchSessionId() {
      return bound_session_id_;
    }
  } threaded_info;

  template <typename Func>
  auto withThisSession(Func &&f) {
    if (auto session_id = threaded_info.fetchSessionId(); session_id)
      return std::forward<Func>(f)(*session_id);

    throw jsonrpc::InternalErrorFault(
        "Internal error. No session was bound to subscription.");
  }
}  // namespace

namespace {
  using namespace kagome::api;

  /**
   * Method to format json-data event into json-string representation.
   * @tparam F is a functor type to be called with correct formatted string
   * @param server pointer to jrpc-server
   * @param logger pointer to logger
   * @param set_id subscription set id
   * @param name is an event name
   * @param value event value to be formatted
   * @param f is a functor, to process the result
   */
  template <typename F>
  inline void forJsonData(std::shared_ptr<JRpcServer> server,
                          kagome::log::Logger logger,
                          uint32_t set_id,
                          std::string_view name,
                          jsonrpc::Value &&value,
                          F &&f) {
    BOOST_ASSERT(server);
    BOOST_ASSERT(logger);
    BOOST_ASSERT(!name.empty());

    jsonrpc::Value::Struct response;
    response["result"] = std::move(value);
    response["subscription"] = makeValue(set_id);

    jsonrpc::Request::Parameters params;
    params.push_back(std::move(response));

    server->processJsonData(name.data(), params, [&](const auto &response) {
      if (response.has_value())
        std::forward<F>(f)(response.value());
      else
        logger->error("process Json data failed => {}", response.error());
    });
  }
  inline void sendEvent(std::shared_ptr<JRpcServer> server,
                        std::shared_ptr<Session> session,
                        kagome::log::Logger logger,
                        uint32_t set_id,
                        std::string_view name,
                        jsonrpc::Value &&value) {
    BOOST_ASSERT(session);
    forJsonData(server,
                logger,
                set_id,
                name,
                std::move(value),
                [session{std::move(session)}](const auto &response) {
                  // Defer sending JSON-RPC event until subscription id is sent.
                  // TODO(turuslan): #1474, refactor jrpc notifications
                  session->post([session, response{std::string{response}}] {
                    session->respond(response);
                  });
                });
  }
}  // namespace

namespace kagome::api {
  KAGOME_DEFINE_CACHE(api_service);

  const std::string kRpcEventRuntimeVersion = "state_runtimeVersion";
  const std::string kRpcEventNewHeads = "chain_newHead";
  const std::string kRpcEventFinalizedHeads = "chain_finalizedHead";
  const std::string kRpcEventSubscribeStorage = "state_storage";

  const std::string kRpcEventUpdateExtrinsic = "author_extrinsicUpdate";

  ApiServiceImpl::ApiServiceImpl(
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
      std::shared_ptr<storage::trie::TrieStorage> trie_storage,
      std::shared_ptr<runtime::Core> core)
      : thread_pool_(std::move(thread_pool)),
        listeners_(std::move(listeners.listeners)),
        server_(std::move(server)),
        logger_{log::createLogger("ApiService", "api")},
        block_tree_{std::move(block_tree)},
        trie_storage_{std::move(trie_storage)},
        core_(std::move(core)),
        subscription_engines_{.storage = std::move(storage_sub_engine),
                              .chain = std::move(chain_sub_engine),
                              .ext = std::move(ext_sub_engine)},
        extrinsic_event_key_repo_{std::move(extrinsic_event_key_repo)} {
    BOOST_ASSERT(thread_pool_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(trie_storage_);
    BOOST_ASSERT(core_);
    BOOST_ASSERT(
        std::all_of(listeners_.cbegin(), listeners_.cend(), [](auto &listener) {
          return listener != nullptr;
        }));
    for (auto &processor : processors.processors) {
      BOOST_ASSERT(processor != nullptr);
      processor->registerHandlers();
    }

    BOOST_ASSERT(app_state_manager);
    app_state_manager->takeControl(*this);

    BOOST_ASSERT(subscription_engines_.chain);
    BOOST_ASSERT(subscription_engines_.storage);
    BOOST_ASSERT(subscription_engines_.ext);
    BOOST_ASSERT(extrinsic_event_key_repo_);
  }

  jsonrpc::Value ApiServiceImpl::createStateStorageEvent(
      const std::vector<
          std::pair<common::Buffer, std::optional<common::Buffer>>>
          &key_value_pairs,
      const primitives::BlockHash &block) {
    /// TODO(iceseer): PRE-475 make event notification depending
    /// in packs blocks, to batch them in a single message Because
    /// of a spec, we can send an array of changes in a single
    /// message. We can receive here a pack of events and format
    /// them in a single json message.

    jsonrpc::Value::Array changes;
    changes.reserve(key_value_pairs.size());
    for (auto &[key, value] : key_value_pairs) {
      changes.emplace_back(jsonrpc::Value{jsonrpc::Value::Array{
          api::makeValue(key),
          value.has_value() ? api::makeValue(hex_lower_0x(value.value()))
                            : api::makeValue(std::nullopt)}});
    }

    jsonrpc::Value::Struct result;
    result["changes"] = std::move(changes);
    result["block"] = api::makeValue(hex_lower_0x(block));

    return result;
  }

  bool ApiServiceImpl::prepare() {
    for (const auto &listener : listeners_) {
      auto on_new_session =
          [wp = weak_from_this()](const sptr<Session> &session) mutable {
            auto self = wp.lock();
            if (!self) {
              return;
            }

#define UNWRAP_WEAK_PTR(callback)  \
  [wp](auto &&...params) mutable { \
    if (auto self = wp.lock()) {   \
      self->callback(params...);   \
    }                              \
  }

            if (SessionType::kWs == session->type()) {
              auto session_context =
                  self->storeSessionWithId(session->id(), session);
              BOOST_ASSERT(session_context);
              session_context->storage_sub->setCallback(
                  UNWRAP_WEAK_PTR(onStorageEvent));
              session_context->chain_sub->setCallback(
                  UNWRAP_WEAK_PTR(onChainEvent));
              session_context->ext_sub->setCallback(
                  UNWRAP_WEAK_PTR(onExtrinsicEvent));
            }

            session->connectOnRequest(UNWRAP_WEAK_PTR(onSessionRequest));
            session->connectOnCloseHandler(UNWRAP_WEAK_PTR(onSessionClose));
          };
#undef UNWRAP_WEAK_PTR

      listener->setHandlerForNewSession(std::move(on_new_session));
    }
    return true;
  }  // namespace kagome::api

  bool ApiServiceImpl::start() {
    thread_pool_->start();
    SL_DEBUG(logger_, "API Service started");
    return true;
  }

  void ApiServiceImpl::stop() {
    thread_pool_->stop();
    SL_DEBUG(logger_, "API Service stopped");
  }

  std::shared_ptr<ApiServiceImpl::SessionSubscriptions>
  ApiServiceImpl::storeSessionWithId(Session::SessionId id,
                                     const std::shared_ptr<Session> &session) {
    std::lock_guard guard(subscribed_sessions_cs_);
    auto &&[it, inserted] = subscribed_sessions_.emplace(
        id,
        std::make_shared<ApiServiceImpl::SessionSubscriptions>(
            ApiServiceImpl::SessionSubscriptions{
                .storage_sub = std::make_shared<StorageEventSubscriber>(
                    subscription_engines_.storage, session),
                .chain_sub = std::make_shared<ChainEventSubscriber>(
                    subscription_engines_.chain, session),
                .ext_sub = std::make_shared<ExtrinsicEventSubscriber>(
                    subscription_engines_.ext, session),
                .messages = {}}));

    BOOST_ASSERT(inserted);
    return it->second;
  }

  void ApiServiceImpl::removeSessionById(Session::SessionId id) {
    std::lock_guard guard(subscribed_sessions_cs_);
    subscribed_sessions_.erase(id);
  }

  outcome::result<ApiServiceImpl::PubsubSubscriptionId>
  ApiServiceImpl::subscribeSessionToKeys(
      const std::vector<common::Buffer> &keys) {
    return withThisSession(
        [&](kagome::api::Session::SessionId tid)
            -> outcome::result<ApiServiceImpl::PubsubSubscriptionId> {
          return withSession(
              tid,
              [&](SessionSubscriptions &session_context)
                  -> outcome::result<ApiServiceImpl::PubsubSubscriptionId> {
                auto &session = session_context.storage_sub;
                const auto id = session->generateSubscriptionSetId();
                const auto &best_block_hash = block_tree_->bestLeaf().hash;
                const auto &header =
                    block_tree_->getBlockHeader(best_block_hash);
                BOOST_ASSERT(header.has_value());
                auto persistent_batch = trie_storage_->getPersistentBatchAt(
                    header.value().state_root, kNoFn);
                if (!persistent_batch.has_value()) {
                  SL_ERROR(logger_,
                           "Failed to get storage state for block {}, required "
                           "to subscribe an RPC session to some storage keys.",
                           best_block_hash);
                  return persistent_batch.as_failure();
                }

                auto &batch = persistent_batch.value();

                session_context.messages = uploadMessagesListFromCache();

                std::vector<
                    std::pair<common::Buffer, std::optional<common::Buffer>>>
                    pairs;
                pairs.reserve(keys.size());

                for (auto &key : keys) {
                  session->subscribe(id, key);

                  auto value_opt_res = batch->tryGet(key);
                  if (value_opt_res.has_value()) {
                    pairs.emplace_back(key, value_opt_res.value());
                  }
                }

                forJsonData(server_,
                            logger_,
                            id,
                            kRpcEventSubscribeStorage,
                            createStateStorageEvent(pairs, best_block_hash),
                            [&](const auto &result) {
                              session_context.messages->emplace_back(
                                  uploadFromCache(result.data()));
                            });

                return static_cast<PubsubSubscriptionId>(id);
              });
        });
  }

  outcome::result<ApiServiceImpl::PubsubSubscriptionId>
  ApiServiceImpl::subscribeFinalizedHeads() {
    return withThisSession([&](kagome::api::Session::SessionId tid) {
      return withSession(tid, [&](SessionSubscriptions &session_context) {
        auto &session = session_context.chain_sub;
        const auto id = session->generateSubscriptionSetId();
        session->subscribe(id,
                           primitives::events::ChainEventType::kFinalizedHeads);

        auto header =
            block_tree_->getBlockHeader(block_tree_->getLastFinalized().hash);
        if (!header.has_error()) {
          session_context.messages = uploadMessagesListFromCache();
          forJsonData(server_,
                      logger_,
                      id,
                      kRpcEventFinalizedHeads,
                      makeValue(header.value()),
                      [&](const auto &result) {
                        session_context.messages->emplace_back(
                            uploadFromCache(result.data()));
                      });
        } else {
          logger_->error(
              "Request block header of the last finalized "
              "failed with error: "
              "{}",
              header.error());
        }
        return static_cast<PubsubSubscriptionId>(id);
      });
    });
  }

  outcome::result<bool> ApiServiceImpl::unsubscribeFinalizedHeads(
      PubsubSubscriptionId subscription_id) {
    return withThisSession([&](kagome::api::Session::SessionId tid) {
      return withSession(tid, [&](SessionSubscriptions &session_context) {
        auto &session = session_context.chain_sub;
        return session->unsubscribe(subscription_id);
      });
    });
  }

  outcome::result<ApiServiceImpl::PubsubSubscriptionId>
  ApiServiceImpl::subscribeNewHeads() {
    return withThisSession([&](kagome::api::Session::SessionId tid) {
      return withSession(tid, [&](SessionSubscriptions &session_context) {
        auto &session = session_context.chain_sub;
        const auto id = session->generateSubscriptionSetId();
        session->subscribe(id, primitives::events::ChainEventType::kNewHeads);

        auto header = block_tree_->getBlockHeader(block_tree_->bestLeaf().hash);
        if (!header.has_error()) {
          session_context.messages = uploadMessagesListFromCache();
          forJsonData(server_,
                      logger_,
                      id,
                      kRpcEventNewHeads,
                      makeValue(header.value()),
                      [&](const auto &result) {
                        session_context.messages->emplace_back(
                            uploadFromCache(result.data()));
                      });
        } else {
          logger_->error(
              "Request block header of the deepest leaf failed with error: {}",
              header.error());
        }
        return static_cast<PubsubSubscriptionId>(id);
      });
    });
  }

  outcome::result<bool> ApiServiceImpl::unsubscribeNewHeads(
      PubsubSubscriptionId subscription_id) {
    return withThisSession([&](kagome::api::Session::SessionId tid) {
      return withSession(tid, [&](SessionSubscriptions &session_context) {
        auto &session = session_context.chain_sub;
        return session->unsubscribe(subscription_id);
      });
    });
  }

  outcome::result<ApiServiceImpl::PubsubSubscriptionId>
  ApiServiceImpl::subscribeRuntimeVersion() {
    return withThisSession([&](kagome::api::Session::SessionId tid) {
      return withSession(tid, [&](SessionSubscriptions &session_context) {
        auto &session = session_context.chain_sub;
        const auto id = session->generateSubscriptionSetId();
        session->subscribe(
            id, primitives::events::ChainEventType::kFinalizedRuntimeVersion);

        auto version_res = core_->version(block_tree_->getLastFinalized().hash);
        if (version_res.has_value()) {
          const auto &version = version_res.value();
          session_context.messages = uploadMessagesListFromCache();
          forJsonData(server_,
                      logger_,
                      id,
                      kRpcEventRuntimeVersion,
                      makeValue(version),
                      [&](const auto &result) {
                        session_context.messages->emplace_back(
                            uploadFromCache(result.data()));
                      });
        }
        return static_cast<PubsubSubscriptionId>(id);
      });
    });
  }

  outcome::result<bool> ApiServiceImpl::unsubscribeRuntimeVersion(
      PubsubSubscriptionId subscription_id) {
    return withThisSession([&](kagome::api::Session::SessionId tid) {
      return withSession(tid, [&](SessionSubscriptions &session_context) {
        auto &session = session_context.chain_sub;
        return session->unsubscribe(subscription_id);
      });
    });
  }

  outcome::result<ApiServiceImpl::PubsubSubscriptionId>
  ApiServiceImpl::subscribeForExtrinsicLifecycle(
      const primitives::Transaction::Hash &tx_hash) {
    return withThisSession([&](kagome::api::Session::SessionId tid) {
      return withSession(tid, [&](SessionSubscriptions &session_context) {
        auto &session_sub = session_context.ext_sub;
        const auto sub_id = session_sub->generateSubscriptionSetId();
        const auto key = extrinsic_event_key_repo_->add(tx_hash);
        session_sub->subscribe(sub_id, key);

        return static_cast<PubsubSubscriptionId>(sub_id);
      });
    });
  }

  outcome::result<bool> ApiServiceImpl::unsubscribeFromExtrinsicLifecycle(
      PubsubSubscriptionId subscription_id) {
    return withThisSession([&](kagome::api::Session::SessionId tid) {
      return withSession(tid, [&](SessionSubscriptions &session_context) {
        auto &session = session_context.ext_sub;
        return session->unsubscribe(subscription_id);
      });
    });
  }

  outcome::result<bool> ApiServiceImpl::unsubscribeSessionFromIds(
      const std::vector<PubsubSubscriptionId> &subscription_ids) {
    return withThisSession([&](kagome::api::Session::SessionId tid) {
      return withSession(tid, [&](SessionSubscriptions &session_context) {
        auto &session = session_context.storage_sub;
        for (auto id : subscription_ids) session->unsubscribe(id);
        return true;
      });
    });
  }

  void ApiServiceImpl::onSessionRequest(std::string_view request,
                                        std::shared_ptr<Session> session) {
    auto thread_session_auto_release = [](void *) {
      threaded_info.releaseSessionId();
    };

    threaded_info.storeSessionId(session->id());

    /**
     * Unique ptr object to autorelease sessions.
     * 0xff if a random not null value to jump internal nullptr check.
     */
    std::unique_ptr<void, decltype(thread_session_auto_release)>
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        thread_session_keeper(reinterpret_cast<void *>(0xff),
                              std::move(thread_session_auto_release));

    // TODO(kamilsa): remove that string replacement when
    // https://github.com/soramitsu/kagome/issues/572 resolved
    std::string str_request(request);
    boost::replace_first(str_request, "\"params\":null", "\"params\":[null]");

    // process new request
    server_->processData(str_request, [&](std::string_view response) mutable {
      // process response
      session->respond(response);
    });

    try {
      withSession(session->id(), [&](SessionSubscriptions &session_context) {
        if (session_context.messages)
          for (auto &msg : *session_context.messages) {
            BOOST_ASSERT(msg);
            session->respond(*msg);
          }

        session_context.messages.reset();
      });
    } catch (jsonrpc::InternalErrorFault &e) {
      SL_DEBUG(logger_, "Internal jsonrpc error: {}", e.what());
    }
  }

  void ApiServiceImpl::onSessionClose(Session::SessionId id, SessionType) {
    removeSessionById(id);
  }

  void ApiServiceImpl::onStorageEvent(SubscriptionSetId set_id,
                                      SessionPtr &session,
                                      const Buffer &key,
                                      const std::optional<Buffer> &data,
                                      const common::Hash256 &block) {
    sendEvent(server_,
              session,
              logger_,
              set_id,
              kRpcEventSubscribeStorage,
              createStateStorageEvent({{key, data}}, block));
  }

  void ApiServiceImpl::onChainEvent(
      SubscriptionSetId set_id,
      SessionPtr &session,
      primitives::events::ChainEventType event_type,
      const primitives::events::ChainEventParams &event_params) {
    std::string_view name;
    switch (event_type) {
      case primitives::events::ChainEventType::kNewHeads: {
        name = kRpcEventNewHeads;
      } break;
      case primitives::events::ChainEventType::kFinalizedHeads: {
        name = kRpcEventFinalizedHeads;
      } break;
      case primitives::events::ChainEventType::kFinalizedRuntimeVersion: {
        name = kRpcEventRuntimeVersion;
      } break;
      case primitives::events::ChainEventType::kNewRuntime:
        return;
      default:
        BOOST_ASSERT(!"Unknown chain event");
        return;
    }

    BOOST_ASSERT(!name.empty());
    sendEvent(
        server_, session, logger_, set_id, name, api::makeValue(event_params));
  }

  void ApiServiceImpl::onExtrinsicEvent(
      SubscriptionSetId set_id,
      SessionPtr &session,
      primitives::events::SubscribedExtrinsicId ext_id,
      const primitives::events::ExtrinsicLifecycleEvent &params) {
    sendEvent(server_,
              session,
              logger_,
              set_id,
              kRpcEventUpdateExtrinsic,
              api::makeValue(params));
  }

}  // namespace kagome::api
