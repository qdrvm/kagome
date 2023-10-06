/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/noncopyable.hpp>
#include <memory>

namespace kagome::api {
  class JRpcServer;

  /**
   * @class JRpcProcessor is base class for JSON RPC processors
   */
  class JRpcProcessor : private boost::noncopyable {
   public:
    virtual ~JRpcProcessor() = default;

    /**
     * @brief registers callbacks for jrpc request
     */
    virtual void registerHandlers() = 0;
  };
}  // namespace kagome::api
