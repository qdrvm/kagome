/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/service/api_service.hpp"

#include "api/jrpc/jrpc_processor.hpp"

namespace kagome::api {

  ApiService::ApiService(
      std::vector<std::shared_ptr<Listener>> listeners,
      std::shared_ptr<JRpcServer> server,
      gsl::span<std::shared_ptr<JRpcProcessor>> processors)
      : listeners_(std::move(listeners)),
        server_(std::move(server)),
        logger_{common::createLogger("Api service")} {
    for (const auto& listener : listeners_)
	{
		BOOST_ASSERT(listener != nullptr);
	}
    for(auto& processor: processors) {
      BOOST_ASSERT(processor != nullptr);
      processor->registerHandlers();
    }
  }

  void ApiService::start() {
    // handle new session
    for (const auto& listener : listeners_)
	{
		listener->start(
			[self = shared_from_this()](
				const sptr<Session>& session
			) mutable {
				session->connectOnRequest(
					[self](
						std::string_view request,
						std::shared_ptr<Session> session
					) mutable {
						// process new request
						self->server_->processData(
							std::string(request),
							[session = std::move(session)](
								const std::string& response
							) mutable {
								// process response
								session->respond(response);
							}
						);
					}
				);
			}
		);
	}
    logger_->debug("Service started");
  }

  void ApiService::stop() {
	  for (const auto& listener : listeners_)
	  {
		  listener->stop();
	  }
  }

}  // namespace kagome::api
