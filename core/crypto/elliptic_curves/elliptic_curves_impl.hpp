/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/elliptic_curves.hpp"

namespace kagome::crypto {

  class EllipticCurvesImpl : public EllipticCurves {
   public:
    outcome::result<common::Buffer> bls12_381_multi_miller_loop(
        common::BufferView a, common::BufferView b) const override;

    outcome::result<common::Buffer> bls12_381_final_exponentiation(
        common::BufferView f) const override;

    outcome::result<common::Buffer> bls12_381_mul_projective_g1(
        common::BufferView base, common::BufferView scalar) const override;

    outcome::result<common::Buffer> bls12_381_mul_projective_g2(
        common::BufferView base, common::BufferView scalar) const override;

    outcome::result<common::Buffer> bls12_381_msm_g1(
        common::BufferView bases, common::BufferView scalars) const override;

    outcome::result<common::Buffer> bls12_381_msm_g2(
        common::BufferView bases, common::BufferView scalars) const override;

    outcome::result<common::Buffer>
    ed_on_bls12_381_bandersnatch_sw_mul_projective(
        common::BufferView base, common::BufferView scalar) const override;
  };

  enum class EllipticCurvesError { ARKWORKS_RETURN_ERROR = 1 };

}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, EllipticCurvesError);
