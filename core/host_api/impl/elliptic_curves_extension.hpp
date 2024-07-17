/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <future>
#include <optional>
#include <queue>

#include "log/logger.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/types.hpp"

namespace kagome::crypto {
  class EllipticCurves;
}

namespace kagome::host_api {
  /**
   * Implements extension functions related to ecliptic curves
   */
  class EllipticCurvesExtension {
   public:
    EllipticCurvesExtension(
        std::shared_ptr<const runtime::MemoryProvider> memory_provider,
        std::shared_ptr<const crypto::EllipticCurves> elliptic_curves);

    // -------------------- bls12_381 methods --------------------

    /// @see HostApi::ext_elliptic_curves_bls12_381_multi_miller_loop_version_1
    runtime::WasmSpan ext_elliptic_curves_bls12_381_multi_miller_loop_version_1(
        runtime::WasmSpan a, runtime::WasmSpan b) const;

    /// @see
    /// HostApi::ext_elliptic_curves_bls12_381_final_exponentiation_version_1
    runtime::WasmSpan
    ext_elliptic_curves_bls12_381_final_exponentiation_version_1(
        runtime::WasmSpan f) const;

    /// @see HostApi::ext_elliptic_curves_bls12_381_mul_projective_g1_version_1
    runtime::WasmSpan ext_elliptic_curves_bls12_381_mul_projective_g1_version_1(
        runtime::WasmSpan base, runtime::WasmSpan scalar) const;

    /// @see HostApi::ext_elliptic_curves_bls12_381_mul_projective_g2_version_1
    runtime::WasmSpan ext_elliptic_curves_bls12_381_mul_projective_g2_version_1(
        runtime::WasmSpan base, runtime::WasmSpan scalar) const;

    /// @see HostApi::ext_elliptic_curves_bls12_381_msm_g1_version_1
    runtime::WasmSpan ext_elliptic_curves_bls12_381_msm_g1_version_1(
        runtime::WasmSpan bases, runtime::WasmSpan scalars) const;

    /// @see HostApi::ext_elliptic_curves_bls12_381_msm_g2_version_1
    runtime::WasmSpan ext_elliptic_curves_bls12_381_msm_g2_version_1(
        runtime::WasmSpan bases, runtime::WasmSpan scalars) const;

    /// @see
    /// HostApi::ext_elliptic_curves_ed_on_bls12_381_bandersnatch_sw_mul_projective_version_1
    runtime::WasmSpan
    ext_elliptic_curves_ed_on_bls12_381_bandersnatch_sw_mul_projective_version_1(
        runtime::WasmSpan base, runtime::WasmSpan scalar) const;

   private:
    runtime::Memory &getMemory() const {
      return memory_provider_->getCurrentMemory()->get();
    }

    log::Logger logger_;
    std::shared_ptr<const runtime::MemoryProvider> memory_provider_;
    std::shared_ptr<const crypto::EllipticCurves> elliptic_curves_;
  };
}  // namespace kagome::host_api
