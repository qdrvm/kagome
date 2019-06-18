/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_TRANSPORT_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_TRANSPORT_HPP

#include <boost/signals2/signal.hpp>

namespace kagome::service {
  class JsonTransport {
   public:
    virtual ~JsonTransport() = default;

    inline auto & dataReceived() { return data_received_signal_; }

    virtual void sendResponse(std::string_view data) = 0;
   private:
    boost::signals2::signal<void (const std::string &data)> data_received_signal_;
  };
}

#endif //KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_TRANSPORT_HPP
