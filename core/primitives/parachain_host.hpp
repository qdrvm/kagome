/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <vector>

#include <boost/variant.hpp>
#include <optional>

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "network/types/collator_messages.hpp"

namespace kagome::primitives::parachain {
  /**
   * @brief ValidatorId primitive is an ed25519 or sr25519 public key
   */
  using ValidatorId = network::ValidatorId;

  /**
   * @brief ParachainId primitive is an uint32_t
   */
  using ParaId = network::ParachainId;

  /**
   * @brief Relay primitive is empty in polkadot for now
   */
  struct Relay {};
  SCALE_EMPTY_CODER(Relay);

  /**
   * @brief Parachain primitive
   */
  using Parachain = ParaId;

  /**
   * @brief Chain primitive
   */
  using Chain = boost::variant<Relay, Parachain>;

  /**
   * @brief DutyRoster primitive
   */
  using DutyRoster = std::vector<Chain>;
}  // namespace kagome::primitives::parachain
