/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BABE_API_HPP
#define KAGOME_CORE_RUNTIME_BABE_API_HPP

#include <outcome/outcome.hpp>

#include "primitives/babe_configuration.hpp"

namespace kagome::runtime {

  /**
   * Api to invoke runtime entries related for Babe algorithm
   */
  class BabeApi {
   public:
    virtual ~BabeApi() = default;

    /**
     * Get configuration for the babe
     * @return Babe configuration
     */
    virtual outcome::result<primitives::BabeConfiguration> configuration(
        const primitives::BlockHash &block) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_BABE_API_HPP
