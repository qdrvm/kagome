/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STATE_REQUEST_HPP
#define KAGOME_STATE_REQUEST_HPP

#include "common/buffer.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  struct StateRequest {
    // Block header hash.
    primitives::BlockHash hash;
    // Start from this key.
    // Multiple keys used for nested state start.
    std::vector<common::Buffer> start;
    // if 'true' indicates that response should contain raw key-values, rather
    // than proof.
    bool no_proof;
  };
}  // namespace kagome::network

#endif
