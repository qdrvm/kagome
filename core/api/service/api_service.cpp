/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "api/service/api_service.hpp"

#include "api/jrpc/jrpc_processor.hpp"

namespace kagome::api {

  ApiService::ApiService(std::vector<std::shared_ptr<Listener>> listeners,
                         std::shared_ptr<JRpcServer> server,
                         gsl::span<std::shared_ptr<JRpcProcessor>> processors)
      : listeners_(std::move(listeners)),
        server_(std::move(server)),
        logger_{common::createLogger("Api service")} {
    for (const auto &listener : listeners_) {
      BOOST_ASSERT(listener != nullptr);
    }
    for (auto &processor : processors) {
      BOOST_ASSERT(processor != nullptr);
      processor->registerHandlers();
    }
  }

  void ApiService::start() {
    // handle new session
    for (const auto &listener : listeners_) {
      listener->start(
          [self = shared_from_this()](const sptr<Session> &session) mutable {
            session->connectOnRequest(
                [self](std::string_view request,
                       std::shared_ptr<Session> session) mutable {
                  // process new request
                  self->server_->processData(
                      std::string(request),
                      [session = std::move(session)](
                          const std::string &response) mutable {
                        // process response
                        session->respond(response);
                      });
                });
          });
    }
    logger_->debug("Service started");
  }

  void ApiService::stop() {
    for (const auto &listener : listeners_) {
      listener->stop();
    }
  }

}  // namespace kagome::api
