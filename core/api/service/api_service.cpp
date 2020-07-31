/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/api_service.hpp"

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
}  // namespace

namespace kagome::api {

  ApiService::ApiService(
      const std::shared_ptr<application::AppStateManager> &app_state_manager,
      std::shared_ptr<api::RpcThreadPool> thread_pool,
      std::vector<std::shared_ptr<Listener>> listeners,
      std::shared_ptr<JRpcServer> server,
      gsl::span<std::shared_ptr<JRpcProcessor>> processors,
      SubscriptionEnginePtr subscription_engine)
      : thread_pool_(std::move(thread_pool)),
        listeners_(std::move(listeners)),
        server_(std::move(server)),
        logger_{common::createLogger("Api service")},
        subscription_engine_(std::move(subscription_engine)) {
    BOOST_ASSERT(thread_pool_);
    for ([[maybe_unused]] const auto &listener : listeners_) {
      BOOST_ASSERT(listener != nullptr);
    }
    for (auto &processor : processors) {
      BOOST_ASSERT(processor != nullptr);
      processor->registerHandlers();
    }

    BOOST_ASSERT(app_state_manager);
    app_state_manager->takeControl(*this);
  }

  void ApiService::prepare() {
    for (const auto &listener : listeners_) {
      auto on_new_session =
          [wp = weak_from_this()](const sptr<Session> &session) mutable {
            auto self = wp.lock();
            if (!self) {
              return;
            }

            if (SessionType::kWs == session->type()) {
              auto subscribed_session =
                  self->storeSessionWithId(session->id(), session);
              subscribed_session->setCallback([wp](SessionPtr &session,
                                                    const auto &key,
                                                    const auto &data,
                                                    const auto &block) {
                if (auto self = wp.lock()) {
                  jsonrpc::Value::Array out_data;
                  out_data.emplace_back(api::makeValue(key));
                  out_data.emplace_back(api::makeValue(data));

                  /// TODO(iceseer): PRE-475 make event notofication depending in
                  /// blocks, to butch them in a single message

                  jsonrpc::Value::Struct result;
                  result["changes"] = std::move(out_data);
                  result["block"] = api::makeValue(block);

                  jsonrpc::Value::Struct params;
                  params["result"] = std::move(result);
                  params["subscription"] = 0;

                  self->server_->processJsonData(
                      params, [&](const std::string &response) {
                        session->respond(response);
                      });
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
                  std::unique_ptr<void, decltype(thread_session_auto_release)>

                  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                  thread_session_keeper(reinterpret_cast<void *>(0xff),
                                        std::move(thread_session_auto_release));

                  // process new request
                  self->server_->processData(
                      std::string(request),
                      [session = std::move(session)](
                          const std::string &response) mutable {
                        // process response
                        session->respond(response);
                      });
                });

            session->connectOnCloseHandler(
                [wp](Session::SessionId id, SessionType /*type*/) {
                  if (auto self = wp.lock()) self->removeSessionById(id);
                });
          };

      listener->setHandlerForNewSession(std::move(on_new_session));
    }
  }

  ApiService::SubscribedSessionPtr ApiService::findSessionById(
      Session::SessionId id) {
    std::lock_guard guard(subscribed_sessions_cs_);
    if (auto it = subscribed_sessions_.find(id);
        subscribed_sessions_.end() != it)
      return it->second;

    return nullptr;
  }

  ApiService::SubscribedSessionPtr ApiService::storeSessionWithId(
      const Session::SessionId id, const std::shared_ptr<Session> &session) {
    std::lock_guard guard(subscribed_sessions_cs_);
    auto &&[it, inserted] = subscribed_sessions_.emplace(
        id,
        std::make_shared<SubscribedSessionType>(subscription_engine_, session));

    BOOST_ASSERT(inserted);
    return std::move(it->second);
  }

  void ApiService::removeSessionById(Session::SessionId id) {
    std::lock_guard guard(subscribed_sessions_cs_);
    subscribed_sessions_.erase(id);
  }

  void ApiService::start() {
    thread_pool_->start();
    logger_->debug("Service started");
  }

  void ApiService::stop() {
    thread_pool_->stop();
    logger_->debug("Service stopped");
  }

  outcome::result<uint32_t> ApiService::subscribe_session_to_keys(
      const std::vector<common::Buffer> &keys) {
    if (auto session_id = threaded_info.fetchSessionId(); session_id) {
      if (auto session = findSessionById(*session_id)) {
        const auto id = session->generate_subscription_set_id();
        for (auto &key : keys) {
          /// TODO(iceseer): PRE-476 make move data to subscription
          session->subscribe(id, key);
        }
        return static_cast<uint32_t>(id);
      }
      throw jsonrpc::InternalErrorFault(
          "Internal error. No session was stored for subscription.");
    }
    throw jsonrpc::InternalErrorFault(
        "Internal error. No session was bound to subscription.");
  }

  outcome::result<void> ApiService::unsubscribe_thread_session_from_ids(
      const std::vector<uint32_t> &subscription_ids) {
    if (auto session_id = threaded_info.fetch_thread_session_id(); session_id) {
      if (auto session = find_session_by_id(*session_id)) {
        for (auto id : subscription_ids)
          session->unsubscribe(id);
        return outcome::success();
      }
      throw jsonrpc::InternalErrorFault(
          "Internal error. No session was stored for subscription.");
    }
    throw jsonrpc::InternalErrorFault(
        "Internal error. No session was binded to subscription.");
  }


}  // namespace kagome::api
