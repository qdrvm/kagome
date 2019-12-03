/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_APPLICATION_KAGOME_APPLICATION_HPP
#define KAGOME_CORE_APPLICATION_KAGOME_APPLICATION_HPP

#include "injector/application_injector.hpp"

namespace kagome::application {

  /**
   * @class KagomeApplication kagome application interface
   */
  class KagomeApplication {
   public:
    virtual ~KagomeApplication() = default;

    /**
     * @brief runs application
     */
    virtual void run() = 0;
  };
}  // namespace kagome::application

#endif  // KAGOME_CORE_APPLICATION_KAGOME_APPLICATION_HPP
