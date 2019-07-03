/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_TRANSPORT_WORKER_API_HPP
#define KAGOME_CORE_API_TRANSPORT_WORKER_API_HPP

#include <boost/signals2/signal.hpp>
#include "api/transport/session.hpp"

namespace kagome::server {
  class WorkerApi {
    template <class T>
    using Signal = boost::signals2::signal<T>;

   public:
    virtual ~WorkerApi() = default;

    inline auto &onRequest() {
      return on_request_;
    }
    inline auto &onResponse() {
      return on_response_;
    }

   protected:
    Signal<void(Session::Id, const std::string &)>
        on_request_;  ///< incoming request obtained
    Signal<void(const std::string &)> on_response_;  ///< response is ready
  };
}  // namespace kagome::server

#endif  // KAGOME_CORE_API_TRANSPORT_WORKER_API_HPP
