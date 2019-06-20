/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_JSON_TRANSPORT_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_JSON_TRANSPORT_HPP

#include <boost/signals2/signal.hpp>
#include <outcome/outcome.hpp>
#include "extrinsics_submission_service/network_address.hpp"

namespace kagome::service {

  // TODO(yuraz): PRE-207 implement (will be implemented in next PR)
  class JsonTransport {
   public:
    using SignalType = void(std::string_view);
    using OnData = boost::signals2::signal<SignalType>;
    using OnResponse = boost::signals2::slot<SignalType>;

    virtual ~JsonTransport() = default;

    JsonTransport();

    /**
     * @brief starts listening
     * @return true if successfully started, false otherwise
     */
    virtual outcome::result<void> start(NetworkAddress address);

    /**
     * @brief stops transport
     */
    virtual void stop();

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
    void processResponse(std::string_view response);

    OnData on_data_;          ///< data received signal
    OnResponse on_response_;  ///< response handler slot
  };
}  // namespace kagome::service

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_JSON_TRANSPORT_HPP
