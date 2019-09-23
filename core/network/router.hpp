/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROUTER_HPP
#define KAGOME_ROUTER_HPP

namespace kagome::network {
  /**
   * Router, which reads and delivers different network messages to the
   * observers, responsible for their processing
   */
  struct Router {
    virtual ~Router() = default;

    /**
     * Start accepting new connections and messages on this router
     */
    virtual void init() = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_ROUTER_HPP
