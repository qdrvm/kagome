/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KAGOME_CONFIG_HPP
#define KAGOME_KAGOME_CONFIG_HPP

#include "primitives/block.hpp"

namespace kagome::application {

  /**
   * Contains configuration parameters of a chain, such as genesis blocks or
   * authority keys
   */
  struct KagomeConfig {
    primitives::Block genesis;
  };

};  // namespace kagome::application

#endif  // KAGOME_KAGOME_CONFIG_HPP
