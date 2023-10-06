/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace kagome::application {

  /// Special mode of running
  class Mode {
   public:
    virtual ~Mode() = default;

    /// Runs application in this mode
    virtual int run() const = 0;
  };

}  // namespace kagome::application
