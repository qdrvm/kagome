/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace kagome::application {

  /**
   * @class KagomeApplication kagome application interface
   */
  class KagomeApplication {
   public:
    virtual ~KagomeApplication() = default;

    /// Prints chain info
    virtual int chainInfo() = 0;

    /// Runs recovery mode
    virtual int recovery() = 0;

    /// Runs node
    virtual void run() = 0;
  };
}  // namespace kagome::application
