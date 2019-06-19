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
    using OnData = boost::signals2::signal<void(const std::string &)>;
    using OnResponse = boost::signals2::slot<void(const std::string &)>;

    virtual ~JsonTransport() = default;

    explicit inline JsonTransport(
        std::function<void(const std::string &)> response_callback)
        : on_response_{response_callback} {}

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
