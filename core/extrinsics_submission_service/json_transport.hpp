/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_JSON_TRANSPORT_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_JSON_TRANSPORT_HPP

#include <boost/signals2/signal.hpp>
#include <outcome/outcome.hpp>

namespace kagome::service {
  class JsonTransport {
   public:
    using SignalType = void(std::string_view);
    using OnData = boost::signals2::signal<SignalType>;
    using OnResponse = boost::signals2::slot<SignalType>;

    virtual ~JsonTransport() = default;

    explicit JsonTransport(std::function<SignalType> response_callback)
        : on_response_{std::move(response_callback)} {}

    /**
     * @brief starts listening
     * @return true if successfully started, false otherwise
     */
    virtual bool start(uint32_t port) = 0;

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
    OnData on_data_;          ///< data received signal
    OnResponse on_response_;  ///< response handler slot
  };
}  // namespace kagome::service

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_JSON_TRANSPORT_HPP
