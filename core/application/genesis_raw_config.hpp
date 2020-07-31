/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_APPLICATION_GENESIS_RAW_CONFIG_HPP
#define KAGOME_CORE_APPLICATION_GENESIS_RAW_CONFIG_HPP

#include "common/buffer.hpp"

namespace kagome::application {

  // configurations from genesis.json lying under "genesis"->"raw" key
  using GenesisRawConfig =
      std::vector<std::pair<common::Buffer, common::Buffer>>;

}  // namespace kagome::application

#endif  // KAGOME_CORE_APPLICATION_GENESIS_RAW_CONFIG_HPP
