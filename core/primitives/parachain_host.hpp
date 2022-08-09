/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_PARACHAIN_HOST_HPP
#define KAGOME_CORE_PRIMITIVES_PARACHAIN_HOST_HPP

#include <cstdint>
#include <vector>

#include <boost/variant.hpp>
#include <optional>

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace kagome::primitives::parachain {
  /**
   * @brief ValidatorId primitive is an ed25519 or sr25519 public key
   */
  using ValidatorId = common::Blob<32>;

  /**
   * @brief ParachainId primitive is an uint32_t
   */
  using ParaId = uint32_t;

  /**
   * @brief Relay primitive is empty in polkadot for now
   */
  struct Relay {};
  // TODO(yuraz): PRE-152 update Relay content when it updates in polkadot
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

#endif  // KAGOME_CORE_PRIMITIVES_PARACHAIN_HOST_HPP
