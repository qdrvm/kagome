/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/api_service.hpp"

#include <boost/algorithm/string/replace.hpp>

#include "api/jrpc/jrpc_processor.hpp"
#include "api/jrpc/value_converter.hpp"

namespace {
  thread_local class {
    boost::optional<kagome::api::Session::SessionId> bound_session_id_ =
        boost::none;

   public:
    void storeSessionId(kagome::api::Session::SessionId id) {
      bound_session_id_ = id;
    }
    void releaseSessionId() {
      bound_session_id_ = boost::none;
    }
    boost::optional<kagome::api::Session::SessionId> fetchSessionId() {
      return bound_session_id_;
    }
  } threaded_info;

  template <typename Func>
  auto for_this_session(Func &&f) {
    if (auto session_id = threaded_info.fetchSessionId(); session_id)
      return std::forward<Func>(f)(*session_id);

    throw jsonrpc::InternalErrorFault(
        "Internal error. No session was binded to subscription.");
  }
}  // namespace

namespace {
  using namespace kagome::api;
  template <typename F>
  inline void forJsonData(std::shared_ptr<JRpcServer> server,
                          kagome::common::Logger logger,
                          uint32_t set_id,
                          std::string_view name,
                          jsonrpc::Value &&value,
                          F &&f) {
    BOOST_ASSERT(server);
    BOOST_ASSERT(logger);
    BOOST_ASSERT(!name.empty());

    jsonrpc::Value::Struct response;
    response["result"] = std::move(value);
    response["subscription"] = kagome::api::makeValue(set_id);

    jsonrpc::Request::Parameters params;
    params.push_back(std::move(response));

    server->processJsonData(name.data(), params, [&](const auto &response) {
      if (response.has_value())
        std::forward<F>(f)(response.value());
      else
        logger->error("process Json data failed => {}",
                      response.error().message());
    });
  }
  inline void sendEvent(std::shared_ptr<JRpcServer> server,
                        std::shared_ptr<Session> session,
                        kagome::common::Logger logger,
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
                  session->respond(response);
                });
  }
}  // namespace

namespace kagome::api {
  KAGOME_DEFINE_CACHE(api_service);

  const std::string kRpcEventRuntimeVersion = "UnknownTag";
  const std::string kRpcEventNewHeads = "chain_newHead";
  const std::string kRpcEventFinalizedHeads = "chain_finalizedHead";

  ApiService::ApiService(
      const std::shared_ptr<application::AppStateManager> &app_state_manager,
      std::shared_ptr<api::RpcThreadPool> thread_pool,
      std::vector<std::shared_ptr<Listener>> listeners,
      std::shared_ptr<JRpcServer> server,
      const std::vector<std::shared_ptr<JRpcProcessor>> &processors,
      SubscriptionEnginePtr subscription_engine,
      subscriptions::EventsSubscriptionEnginePtr events_engine,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<storage::trie::TrieStorage> trie_storage)
      : thread_pool_(std::move(thread_pool)),
        listeners_(std::move(listeners)),
        server_(std::move(server)),
        logger_{common::createLogger("Api service")},
        block_tree_{std::move(block_tree)},
        trie_storage_{std::move(trie_storage)},
        subscription_engines_{.storage = std::move(subscription_engine),
                              .events = std::move(events_engine)} {
    BOOST_ASSERT(thread_pool_);
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(trie_storage_);
    for ([[maybe_unused]] const auto &listener : listeners_) {
      BOOST_ASSERT(listener != nullptr);
    }
    for (auto &processor : processors) {
      BOOST_ASSERT(processor != nullptr);
      processor->registerHandlers();
    }

    BOOST_ASSERT(app_state_manager);
    app_state_manager->takeControl(*this);

    BOOST_ASSERT(subscription_engines_.events);
    BOOST_ASSERT(subscription_engines_.storage);
  }

  bool ApiService::prepare() {
    for (const auto &listener : listeners_) {
      auto on_new_session = [wp = weak_from_this()](
                                const sptr<Session> &session) mutable {
        auto self = wp.lock();
        if (!self) {
          return;
        }

        if (SessionType::kWs == session->type()) {
          auto session_context =
              self->storeSessionWithId(session->id(), session);
          session_context.storage_subscription->setCallback(
              [wp](uint32_t set_id,
                   SessionPtr &session,
                   const auto &key,
                   const auto &data,
                   const auto &block) {
                if (auto self = wp.lock()) {
                  jsonrpc::Value::Array out_data;
                  out_data.emplace_back(api::makeValue(key));
                  out_data.emplace_back(api::makeValue(hex_lower_0x(data)));

                  /// TODO(iceseer): PRE-475 make event notification depending
                  /// in packs blocks, to batch them in a single message Because
                  /// of a spec, we can send an array of changes in a single
                  /// message. We can receive here a pack of events and format
                  /// them in a single json message.

                  jsonrpc::Value::Struct result;
                  result["changes"] = std::move(out_data);
                  result["block"] = api::makeValue(block);

                  sendEvent(self->server_,
                            session,
                            self->logger_,
                            set_id,
                            "state_storage",
                            std::move(result));
                }
              });

          session_context.events_subscription->setCallback([wp](uint32_t set_id,
                                                                SessionPtr
                                                                    &session,
                                                                const auto &key,
                                                                const auto
                                                                    &header) {
            if (auto self = wp.lock()) {
              std::string_view name;
              if (key == primitives::SubscriptionEventType::kNewHeads)
                name = kRpcEventNewHeads;
              else if (key
                       == primitives::SubscriptionEventType::kFinalizedHeads)
                name = kRpcEventFinalizedHeads;
              else if (key
                       == primitives::SubscriptionEventType::kRuntimeVersion)
                name = kRpcEventRuntimeVersion;

              BOOST_ASSERT(!name.empty());
              sendEvent(self->server_,
                        session,
                        self->logger_,
                        set_id,
                        name,
                        api::makeValue(header));
            }
          });
        }

        session->connectOnRequest(
            [wp](std::string_view request,
                 std::shared_ptr<Session> session) mutable {
              auto self = wp.lock();
              if (not self) return;

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
              boost::replace_all(str_request, " ", "");
              boost::replace_first(
                  str_request, "\"params\":null", "\"params\":[null]");

              // process new request
              self->server_->processData(
                  str_request, [&](const std::string &response) mutable {
                    // process response
                    session->respond(response);
                  });

              try {
                self->for_session(
                    session->id(),
                    [&](SessionExecutionContext &session_context) {
                      if (session_context.messages)
                        for (auto &msg : *session_context.messages) {
                          BOOST_ASSERT(msg);
                          session->respond(*msg);
                        }

                      session_context.messages.reset();
                    });
              } catch (jsonrpc::InternalErrorFault &) {
              }
            });

        session->connectOnCloseHandler(
            [wp](Session::SessionId id, SessionType /*type*/) {
              if (auto self = wp.lock()) self->removeSessionById(id);
            });
      };

      listener->setHandlerForNewSession(std::move(on_new_session));
    }
    return true;
  }

  bool ApiService::start() {
    thread_pool_->start();
    logger_->debug("Service started");
    return true;
  }

  void ApiService::stop() {
    thread_pool_->stop();
    logger_->debug("Service stopped");
  }

  boost::optional<ApiService::SessionExecutionContext>
  ApiService::findSessionById(Session::SessionId id) {
    std::lock_guard guard(subscribed_sessions_cs_);
    if (auto it = subscribed_sessions_.find(id);
        subscribed_sessions_.end() != it)
      return it->second;

    return boost::none;
  }

  ApiService::SessionExecutionContext ApiService::storeSessionWithId(
      Session::SessionId id, const std::shared_ptr<Session> &session) {
    std::lock_guard guard(subscribed_sessions_cs_);
    auto &&[it, inserted] = subscribed_sessions_.emplace(
        id,
        ApiService::SessionExecutionContext{
            .storage_subscription = std::make_shared<SubscribedSessionType>(
                subscription_engines_.storage, session),
            .events_subscription =
                std::make_shared<subscriptions::EventsSubscribedSessionType>(
                    subscription_engines_.events, session)});

    BOOST_ASSERT(inserted);
    return it->second;
  }

  void ApiService::removeSessionById(Session::SessionId id) {
    std::lock_guard guard(subscribed_sessions_cs_);
    subscribed_sessions_.erase(id);
  }

  outcome::result<uint32_t> ApiService::subscribeSessionToKeys(
      const std::vector<common::Buffer> &keys) {
    return for_this_session([&](kagome::api::Session::SessionId tid) {
      return for_session(tid, [&](SessionExecutionContext &session_context) {
        auto &session = session_context.storage_subscription;
        const auto id = session->generateSubscriptionSetId();
        auto persistent_batch = trie_storage_->getPersistentBatch();
        BOOST_ASSERT(persistent_batch.has_value());

        auto &pb = persistent_batch.value();
        BOOST_ASSERT(pb);

        session_context.messages = uploadMessagesListFromCache();
        for (auto &key : keys) {
          /// TODO(iceseer): PRE-476 make move data to subscription
          session->subscribe(id, key);
          if (auto res = pb->get(key); res.has_value()) {
            jsonrpc::Value::Array out_data;
            out_data.emplace_back(api::makeValue(key));
            out_data.emplace_back(api::makeValue(hex_lower_0x(res.value())));

            jsonrpc::Value::Struct r;
            r["changes"] = std::move(out_data);

            forJsonData(server_,
                        logger_,
                        id,
                        kRpcEventFinalizedHeads,
                        std::move(r),
                        [&](const auto &result) {
                          session_context.messages->emplace_back(
                              uploadFromCache(result.data()));
                        });
          }
        }
        return static_cast<uint32_t>(id);
      });
    });
  }

  outcome::result<uint32_t> ApiService::subscribeFinalizedHeads() {
    return for_this_session([&](kagome::api::Session::SessionId tid) {
      return for_session(tid, [&](SessionExecutionContext &session_context) {
        auto &session = session_context.events_subscription;
        const auto id = session->generateSubscriptionSetId();
        session->subscribe(id,
                           primitives::SubscriptionEventType::kFinalizedHeads);

        auto header = block_tree_->getBlockHeader(
            block_tree_->getLastFinalized().block_hash);
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
              "Request block header of the last finalized failed with error: "
              "{}",
              header.error().message());
        }
        return static_cast<uint32_t>(id);
      });
    });
  }

  outcome::result<void> ApiService::unsubscribeFinalizedHeads(
      uint32_t subscription_id) {
    return for_this_session([&](kagome::api::Session::SessionId tid) {
      return for_session(tid, [&](SessionExecutionContext &session_context) {
        auto &session = session_context.events_subscription;
        session->unsubscribe(subscription_id);
        return outcome::success();
      });
    });
  }

  outcome::result<uint32_t> ApiService::subscribeNewHeads() {
    return for_this_session([&](kagome::api::Session::SessionId tid) {
      return for_session(tid, [&](SessionExecutionContext &session_context) {
        auto &session = session_context.events_subscription;
        const auto id = session->generateSubscriptionSetId();
        session->subscribe(id, primitives::SubscriptionEventType::kNewHeads);

        auto header =
            block_tree_->getBlockHeader(block_tree_->deepestLeaf().block_hash);
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
              header.error().message());
        }
        return static_cast<uint32_t>(id);
      });
    });
  }

  outcome::result<void> ApiService::unsubscribeNewHeads(
      uint32_t subscription_id) {
    return for_this_session([&](kagome::api::Session::SessionId tid) {
      return for_session(tid, [&](SessionExecutionContext &session_context) {
        auto &session = session_context.events_subscription;
        session->unsubscribe(subscription_id);
        return outcome::success();
      });
    });
  }

  outcome::result<uint32_t> ApiService::subscribeRuntimeVersion() {
    return for_this_session([&](kagome::api::Session::SessionId tid) {
      return for_session(tid, [&](SessionExecutionContext &session_context) {
        auto &session = session_context.events_subscription;
        const auto id = session->generateSubscriptionSetId();
        session->subscribe(id,
                           primitives::SubscriptionEventType::kRuntimeVersion);

        auto ver = block_tree_->runtimeVersion();
        if (ver) {
          session_context.messages = uploadMessagesListFromCache();
          forJsonData(server_,
                      logger_,
                      id,
                      kRpcEventRuntimeVersion,
                      makeValue(*ver),
                      [&](const auto &result) {
                        session_context.messages->emplace_back(
                            uploadFromCache(result.data()));
                      });
        }
        return outcome::success();
      });
    });
  }

  outcome::result<void> ApiService::unsubscribeRuntimeVersion(
      uint32_t subscription_id) {
    return for_this_session([&](kagome::api::Session::SessionId tid) {
      return for_session(tid, [&](SessionExecutionContext &session_context) {
        auto &session = session_context.events_subscription;
        session->unsubscribe(subscription_id);
        return outcome::success();
      });
    });
  }

  outcome::result<void> ApiService::unsubscribeSessionFromIds(
      const std::vector<uint32_t> &subscription_ids) {
    return for_this_session([&](kagome::api::Session::SessionId tid) {
      return for_session(tid, [&](SessionExecutionContext &session_context) {
        auto &session = session_context.storage_subscription;
        for (auto id : subscription_ids) session->unsubscribe(id);
        return outcome::success();
      });
    });
  }

}  // namespace kagome::api
