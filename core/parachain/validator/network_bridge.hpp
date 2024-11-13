/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <type_traits>

#include "common/main_thread_pool.hpp"

namespace libp2p::connection {
  class Stream;
}  // namespace libp2p::connection

namespace kagome::parachain {

  struct NetworkBridge : std::enable_shared_from_this<NetworkBridge> {
    NetworkBridge(common::MainThreadPool &main_thread_pool,
                  application::AppStateManager &app_state_manager)
        : main_pool_handler_{main_thread_pool.handler(app_state_manager)} {}

    template <typename ResponseType, typename Protocol>
    void send_response(std::shared_ptr<libp2p::connection::Stream> stream,
                       std::shared_ptr<Protocol> protocol,
                       std::shared_ptr<ResponseType> response) {
      REINVOKE(*main_pool_handler_,
               send_response,
               std::move(stream),
               std::move(protocol),
               std::move(response));
      protocol->writeResponseAsync(std::move(stream), std::move(*response));
    }

   private:
    log::Logger logger = log::createLogger("NetworkBridge", "parachain");
    std::shared_ptr<PoolHandler> main_pool_handler_;
  };

}  // namespace kagome::parachain
