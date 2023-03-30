/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_AVAILABILITY_FETCH_FETCH_HPP
#define KAGOME_PARACHAIN_AVAILABILITY_FETCH_FETCH_HPP

#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::parachain {
  /**
   * Fetch chunk for availability bitfield voting.
   */
  class Fetch {
   public:
    virtual ~Fetch() = default;

    virtual void fetch(network::RelayHash const &relay_parent, ValidatorIndex chunk_index,
                       const runtime::OccupiedCore &core,
                       const runtime::SessionInfo &session) = 0;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_AVAILABILITY_FETCH_FETCH_HPP
