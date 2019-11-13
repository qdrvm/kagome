/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONFIGURATION_STORAGE_HPP
#define KAGOME_CONFIGURATION_STORAGE_HPP

#include "primitives/block.hpp"

namespace kagome::application {

  /**
   * Stores configuration of a kagome application and provides convenience
   * methods for accessing config parameters
   */
  class ConfigurationStorage {
   public:
    virtual ~ConfigurationStorage() = default;

    /**
     * @return genesis block of the chain
     */
    virtual const primitives::Block &getGenesis() const = 0;
  };

}  // namespace kagome::application

#endif  // KAGOME_CONFIGURATION_STORAGE_HPP
