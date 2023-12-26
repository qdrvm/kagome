/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "scale/tie.hpp"

#include "/home/di/Projects/bandersnatch_vrfs-crust/bandersnatch_vrfs-cpp/bandersnatch_vrfs_crust.h"  // #include <bandersnatch_vrfs/bandersnatch_vrfs.hpp>
#include "consensus/sassafras/types/authority.hpp"
#include "crypto/bandersnatch/vrf.hpp"
#include "crypto/bandersnatch_types.hpp"

struct bandersnatch_RingVrfContext;
struct bandersnatch_RingProver;

namespace kagome::consensus::sassafras::bandersnatch {

  /// Context used to produce ring signatures.
  class RingContext {
   public:
    static constexpr size_t N = 0;

    friend scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                                 RingContext &x) {
      std::array<uint8_t, N> KZG;
      s >> KZG;

      ::bandersnatch_ring_vrf_context(KZG.data(), KZG.size());
      return s;
    }

    crypto::bandersnatch::vrf::RingProver prover(
        std::vector<crypto::BandersnatchPublicKey> keys,
        sassafras::AuthorityIndex index) const {
      std::vector<const uint8_t *> key_ptrs;
      std::transform(
          keys.begin(),
          keys.end(),
          std::back_inserter(key_ptrs),
          [](const crypto::BandersnatchPublicKey &key) { return key.data(); });
      return ::bandersnatch_ring_prover(
          ptr_, key_ptrs.data(), keys.size(), index);
    }

   private:
    const ::bandersnatch_RingVrfContext *ptr_;
  };

}  // namespace kagome::consensus::sassafras::bandersnatch
