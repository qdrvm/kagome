/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_BEAST_LISTENER_IMPL_HPP
#define KAGOME_CORE_API_TRANSPORT_BEAST_LISTENER_IMPL_HPP

#include "api/transport/listener.hpp"

namespace kagome::api {

  class BeastListener : public Listener {
   public:
    void start(NewSessionHandler on_new_session) override;

    void stop() override;

   private:
    /**
     * @brief accepts incoming connection
     */
    void acceptOnce(
        std::function<void(std::shared_ptr<Session>)> on_new_session) override;
  };

}  // namespace kagome::api

#endif  // KAGOME_CORE_API_TRANSPORT_BEAST_LISTENER_IMPL_HPP
