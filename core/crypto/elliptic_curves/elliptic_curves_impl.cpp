/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/elliptic_curves/elliptic_curves_impl.hpp"

#include "common/buffer.hpp"
#include "common/buffer_view.hpp"

#include <arkworks/arkworks.h>

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, EllipticCurvesError, e) {
  using E = decltype(e);
  switch (e) {
    case E::ARKWORKS_RETURN_ERROR:
      return "Arkworks function call returned error";
  }
  return "unknown error (kagome::crypto::EllipticCurvesError)";
}

namespace kagome::crypto {

  namespace {
    ::BytesVec convert(common::BufferView view) {
      return {.data = const_cast<uint8_t *>(view.data()), .size = view.size()};
    }
    outcome::result<common::Buffer> convert(::Result res) {
      if (res.tag == ::RESULT_OK) {
        // TODO avoid coping to runtime
        common::Buffer buf(res.ok.data, res.ok.data + res.ok.size);
        ::AWCR_deallocate_bytesvec(&res.ok);
        return buf;
      }
      return EllipticCurvesError::ARKWORKS_RETURN_ERROR;
    }
  }  // namespace

  outcome::result<common::Buffer>
  EllipticCurvesImpl::bls12_381_multi_miller_loop(common::BufferView a,
                                                  common::BufferView b) const {
    return convert(::bls12_381_multi_miller_loop(convert(a), convert(b)));
  }

  outcome::result<common::Buffer>
  EllipticCurvesImpl::bls12_381_final_exponentiation(
      common::BufferView f) const {
    return convert(::bls12_381_final_exponentiation(convert(f)));
  }

  outcome::result<common::Buffer>
  EllipticCurvesImpl::bls12_381_mul_projective_g1(
      common::BufferView base, common::BufferView scalar) const {
    return convert(
        ::bls12_381_mul_projective_g1(convert(base), convert(scalar)));
  }

  outcome::result<common::Buffer>
  EllipticCurvesImpl::bls12_381_mul_projective_g2(
      common::BufferView base, common::BufferView scalar) const {
    return convert(
        ::bls12_381_mul_projective_g2(convert(base), convert(scalar)));
  }

  outcome::result<common::Buffer> EllipticCurvesImpl::bls12_381_msm_g1(
      common::BufferView bases, common::BufferView scalars) const {
    return convert(::bls12_381_msm_g1(convert(bases), convert(scalars)));
  }

  outcome::result<common::Buffer> EllipticCurvesImpl::bls12_381_msm_g2(
      common::BufferView bases, common::BufferView scalars) const {
    return convert(::bls12_381_msm_g2(convert(bases), convert(scalars)));
  }

  outcome::result<common::Buffer>
  EllipticCurvesImpl::ed_on_bls12_381_bandersnatch_sw_mul_projective(
      common::BufferView base, common::BufferView scalar) const {
    return convert(::ed_on_bls12_381_bandersnatch_sw_mul_projective(
        convert(base), convert(scalar)));
  }

}  // namespace kagome::crypto
