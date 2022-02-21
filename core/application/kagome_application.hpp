/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_APPLICATION_KAGOME_APPLICATION_HPP
#define KAGOME_CORE_APPLICATION_KAGOME_APPLICATION_HPP

namespace kagome::application {

  /**
   * @class KagomeApplication kagome application interface
   */
  class KagomeApplication {
   public:
    virtual ~KagomeApplication() = default;

    /// Runs recovery mode
    virtual int recovery() = 0;

    /// Runs node
    virtual void run() = 0;
  };
}  // namespace kagome::application

#endif  // KAGOME_CORE_APPLICATION_KAGOME_APPLICATION_HPP
