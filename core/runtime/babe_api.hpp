/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_BABE_API_HPP
#define KAGOME_CORE_RUNTIME_BABE_API_HPP

#include <outcome/outcome.hpp>

#include "primitives/babe_configuration.hpp"
#include "primitives/block_id.hpp"

namespace kagome::runtime {

  class BabeApi {
   public:
    virtual ~BabeApi() = default;

    virtual outcome::result<primitives::BabeConfiguration> configuration() = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_BABE_API_HPP
