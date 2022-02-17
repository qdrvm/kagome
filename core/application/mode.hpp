/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_APPLICATION_MODE
#define KAGOME_APPLICATION_MODE

namespace kagome::application {

  /// Special mode of running
  class Mode {
   public:
    virtual ~Mode() = default;

    /// Runs application in this mode
    virtual int run() const = 0;
  };

}  // namespace kagome::application

#endif  // KAGOME_APPLICATION_MODE
