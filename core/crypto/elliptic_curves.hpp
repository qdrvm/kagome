/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "common/buffer_view.hpp"

namespace kagome::crypto {

  class EllipticCurves {
   public:
    virtual ~EllipticCurves() = default;

    /**
     * Pairing multi Miller loop for BLS12-381.
     * @param a
     *   ArkScale<Vec<ark_ec::bls12::G1Prepared::<ark_bls12_381::Config>>>
     * @param b
     *   ArkScale<Vec<ark_ec::bls12::G1Prepared::<ark_bls12_381::Config>>>
     * @return ArkScale<MillerLoopOutput<Bls12<ark_bls12_381::Config>>>
     */
    virtual outcome::result<common::Buffer> bls12_381_multi_miller_loop(
        common::BufferView a, common::BufferView b) const = 0;

    /**
     * Pairing final exponentiation for BLS12-381.
     * @param f ArkScale<MillerLoopOutput<Bls12<ark_bls12_381::Config>>>
     * @return ArkScale<PairingOutput<Bls12<ark_bls12_381::Config>>>
     */
    virtual outcome::result<common::Buffer> bls12_381_final_exponentiation(
        common::BufferView f) const = 0;

    /**
     * Projective multiplication on G1 for BLS12-381.
     * @param base ArkScaleProjective<ark_bls12_381::G1Projective>
     * @param scalar ArkScale<&[u64]>
     * @return ArkScaleProjective<ark_bls12_381::G1Projective>
     */
    virtual outcome::result<common::Buffer> bls12_381_mul_projective_g1(
        common::BufferView base, common::BufferView scalar) const = 0;

    /**
     * Projective multiplication on G2 for BLS12-381.
     * @param base ArkScaleProjective<ark_bls12_381::G2Projective>
     * @param scalar ArkScale<&[u64]>
     * @return ArkScaleProjective<ark_bls12_381::G2Projective>
     */
    virtual outcome::result<common::Buffer> bls12_381_mul_projective_g2(
        common::BufferView base, common::BufferView scalar) const = 0;

    /**
     * Multi scalar multiplication on G1 for BLS12-381.
     * @param bases ArkScale<&[ark_bls12_381::G1Affine]>
     * @param scalars ArkScale<&[ark_bls12_381::Fr]>
     * @return ArkScaleProjective<ark_bls12_381::G1Projective>
     */
    virtual outcome::result<common::Buffer> bls12_381_msm_g1(
        common::BufferView bases, common::BufferView scalars) const = 0;

    /**
     * Multi scalar multiplication on G2 for BLS12-381.
     * @param bases ArkScale<&[ark_bls12_381::G2Affine]>
     * @param scalars ArkScale<&[ark_bls12_381::Fr]>
     * @return ArkScaleProjective<ark_bls12_381::G2Projective>
     */
    virtual outcome::result<common::Buffer> bls12_381_msm_g2(
        common::BufferView bases, common::BufferView scalars) const = 0;

    /**
     * Short Weierstrass projective multiplication for
     * Ed-on-BLS12-381-Bandersnatch.
     * @param base
     *   ArkScaleProjective<ark_ed_on_bls12_381_bandersnatch::SWProjective>
     * @param scalar ArkScale<&[u64]>
     * @return
     *   ArkScaleProjective<ark_ed_on_bls12_381_bandersnatch::SWProjective>
     */
    virtual outcome::result<common::Buffer>
    ed_on_bls12_381_bandersnatch_sw_mul_projective(
        common::BufferView base, common::BufferView scalar) const = 0;
  };

}  // namespace kagome::crypto
