/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_JSON_TRANSPORT_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_JSON_TRANSPORT_HPP

#include <boost/signals2/signal.hpp>
#include <outcome/outcome.hpp>
#include "api/transport/network_address.hpp"

namespace kagome::api {

  class BasicTransport {
   public:
    virtual ~BasicTransport() = default;

    /**
     * @brief starts listening
     * @return true if successfully started, false otherwise
     */
    virtual void start() = 0;

    /**
     * @brief stops transport
     */
    virtual void stop() = 0;

   private:
    /**
     * @brief processes response
     * @param response data to send
     */
    virtual void processResponse(std::string response) = 0;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_JSON_TRANSPORT_HPP
