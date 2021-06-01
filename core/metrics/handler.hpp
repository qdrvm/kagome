/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_METRICS_HANDLER_HPP
#define KAGOME_CORE_METRICS_HANDLER_HPP

#include <memory>

#include "metrics/session.hpp"

namespace kagome::metrics {

  class Registry;
  class Session;

  /**
   * @brief an interface to add request handler for metrics::Exposer
   * implementation generally will contain metrics serializer
   */
  class Handler {
   public:
    virtual ~Handler() = default;
    /**
     * @brief registers general type metrics registry for metrics collection
     */
    virtual void registerCollectable(Registry &registry) = 0;

    /**
     * @brief main interface for session request handling
     */
    virtual void onSessionRequest(Session::Request request,
                                  std::shared_ptr<Session> session) = 0;
  };

}  // namespace kagome::metrics

#endif  // KAGOME_CORE_METRICS_HANDLER_HPP
