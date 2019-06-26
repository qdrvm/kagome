/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_JSON_TRANSPORT_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_JSON_TRANSPORT_HPP

#include <boost/signals2/signal.hpp>
#include <outcome/outcome.hpp>
#include "api/extrinsic/network_address.hpp"

namespace kagome::api {

  // TODO(yuraz): PRE-207 implement (will be implemented in next PR)
  class BasicTransport {
   public:
    using SignalType = void(const std::string &);
    using OnData = boost::signals2::signal<SignalType>;
    using OnResponse = std::function<SignalType>;

    virtual ~BasicTransport() = default;

    BasicTransport();

    /**
     * @brief starts listening
     * @return true if successfully started, false otherwise
     */
    virtual outcome::result<void> start() = 0;

    /**
     * @brief stops transport
     */
    virtual void stop() = 0;

    /**
     * @return data received signal
     */
    inline OnData &dataReceived() {
      return on_data_;
    }

    /**
     * @return response handler slot
     */
    inline OnResponse &onResponse() {
      return on_response_;
    }

   private:
    /**
     * @brief processes response
     * @param response data to send
     */
    virtual void processResponse(const std::string &response) = 0;

    OnData on_data_;          ///< data received signal
    OnResponse on_response_;  ///< response handler slot
  };
}  // namespace kagome::service

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_JSON_TRANSPORT_HPP
