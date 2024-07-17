/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/elliptic_curves_extension.hpp"

#include "crypto/elliptic_curves.hpp"
#include "log/trace_macros.hpp"
#include "runtime/ptr_size.hpp"

namespace {
  template <typename Format, typename... Args>
  void throw_with_error(const kagome::log::Logger &logger,
                        const Format &format,
                        Args &&...args) {
    auto msg = fmt::vformat(format,
                            fmt::make_format_args(std::forward<Args>(args)...));
    logger->error(msg);
    throw std::runtime_error(msg);
  }
}  // namespace

namespace kagome::host_api {

  EllipticCurvesExtension::EllipticCurvesExtension(
      std::shared_ptr<const runtime::MemoryProvider> memory_provider,
      std::shared_ptr<const crypto::EllipticCurves> elliptic_curves)
      : logger_(log::createLogger("EllipticCurvesExtension",
                                  "elliptic_curves_extension")),
        memory_provider_(std::move(memory_provider)),
        elliptic_curves_(std::move(elliptic_curves)) {
    BOOST_ASSERT(memory_provider_ != nullptr);
    BOOST_ASSERT(elliptic_curves_ != nullptr);
  }

  runtime::WasmSpan EllipticCurvesExtension::
      ext_elliptic_curves_bls12_381_multi_miller_loop_version_1(
          runtime::WasmSpan a, runtime::WasmSpan b) const {
    auto [a_addr, a_len] = runtime::PtrSize(a);
    const auto &a_buf = getMemory().loadN(a_addr, a_len);
    auto [b_addr, b_len] = runtime::PtrSize(b);
    const auto &b_buf = getMemory().loadN(b_addr, b_len);

    auto res = elliptic_curves_->bls12_381_multi_miller_loop(a_buf, b_buf);
    if (not res) {
      throw_with_error(
          logger_, "error 'bls12_381_multi_miller_loop' call: {}", res.error());
    }
    SL_TRACE_FUNC_CALL(logger_, res.value(), a_buf, b_buf);

    return getMemory().storeBuffer(res.value());
  }

  runtime::WasmSpan EllipticCurvesExtension::
      ext_elliptic_curves_bls12_381_final_exponentiation_version_1(
          runtime::WasmSpan f) const {
    auto [addr, len] = runtime::PtrSize(f);
    const auto &buf = getMemory().loadN(addr, len);

    auto res = elliptic_curves_->bls12_381_final_exponentiation(buf);
    if (not res) {
      throw_with_error(logger_,
                       "error 'bls12_381_final_exponentiation' call: {}",
                       res.error());
    }
    SL_TRACE_FUNC_CALL(logger_, res.value(), buf, buf);

    return getMemory().storeBuffer(res.value());
  }

  runtime::WasmSpan EllipticCurvesExtension::
      ext_elliptic_curves_bls12_381_mul_projective_g1_version_1(
          runtime::WasmSpan base_span, runtime::WasmSpan scalar_span) const {
    auto [base_addr, base_len] = runtime::PtrSize(base_span);
    const auto &base = getMemory().loadN(base_addr, base_len);
    auto [scalar_addr, scalar_len] = runtime::PtrSize(scalar_span);
    const auto &scalar = getMemory().loadN(scalar_addr, scalar_len);

    auto res = elliptic_curves_->bls12_381_mul_projective_g1(base, scalar);
    if (not res) {
      throw_with_error(
          logger_, "error 'bls12_381_mul_projective_g1' call: {}", res.error());
    }
    SL_TRACE_FUNC_CALL(logger_, res.value(), base, scalar);

    return getMemory().storeBuffer(res.value());
  }

  runtime::WasmSpan EllipticCurvesExtension::
      ext_elliptic_curves_bls12_381_mul_projective_g2_version_1(
          runtime::WasmSpan base_span, runtime::WasmSpan scalar_span) const {
    auto [base_addr, base_len] = runtime::PtrSize(base_span);
    const auto &base = getMemory().loadN(base_addr, base_len);
    auto [scalar_addr, scalar_len] = runtime::PtrSize(scalar_span);
    const auto &scalar = getMemory().loadN(scalar_addr, scalar_len);

    auto res = elliptic_curves_->bls12_381_mul_projective_g2(base, scalar);
    if (not res) {
      throw_with_error(
          logger_, "error 'bls12_381_mul_projective_g2' call: {}", res.error());
    }
    SL_TRACE_FUNC_CALL(logger_, res.value(), base, scalar);

    return getMemory().storeBuffer(res.value());
  }

  runtime::WasmSpan
  EllipticCurvesExtension::ext_elliptic_curves_bls12_381_msm_g1_version_1(
      runtime::WasmSpan bases_span, runtime::WasmSpan scalars_span) const {
    auto [bases_addr, bases_len] = runtime::PtrSize(bases_span);
    const auto &bases = getMemory().loadN(bases_addr, bases_len);
    auto [scalars_addr, scalars_len] = runtime::PtrSize(scalars_span);
    const auto &scalars = getMemory().loadN(scalars_addr, scalars_len);

    auto res = elliptic_curves_->bls12_381_msm_g1(bases, scalars);
    if (not res) {
      throw_with_error(
          logger_, "error 'bls12_381_msm_g1' call: {}", res.error());
    }
    SL_TRACE_FUNC_CALL(logger_, res.value(), bases, scalars);

    return getMemory().storeBuffer(res.value());
  }

  runtime::WasmSpan
  EllipticCurvesExtension::ext_elliptic_curves_bls12_381_msm_g2_version_1(
      runtime::WasmSpan bases_span, runtime::WasmSpan scalars_span) const {
    auto [bases_addr, bases_len] = runtime::PtrSize(bases_span);
    const auto &bases = getMemory().loadN(bases_addr, bases_len);
    auto [scalars_addr, scalars_len] = runtime::PtrSize(scalars_span);
    const auto &scalars = getMemory().loadN(scalars_addr, scalars_len);

    auto res = elliptic_curves_->bls12_381_msm_g2(bases, scalars);
    if (not res) {
      throw_with_error(
          logger_, "error 'bls12_381_msm_g2' call: {}", res.error());
    }
    SL_TRACE_FUNC_CALL(logger_, res.value(), bases, scalars);

    return getMemory().storeBuffer(res.value());
  }

  runtime::WasmSpan EllipticCurvesExtension::
      ext_elliptic_curves_ed_on_bls12_381_bandersnatch_sw_mul_projective_version_1(
          runtime::WasmSpan data1, runtime::WasmSpan data2) const {
    auto [addr1, len1] = runtime::PtrSize(data1);
    const auto &buf1 = getMemory().loadN(addr1, len1);
    auto [addr2, len2] = runtime::PtrSize(data2);
    const auto &buf2 = getMemory().loadN(addr2, len2);

    auto res = elliptic_curves_->ed_on_bls12_381_bandersnatch_sw_mul_projective(
        buf1, buf2);
    if (not res) {
      throw_with_error(
          logger_,
          "error 'ed_on_bls12_381_bandersnatch_sw_mul_projective' call: {}",
          res.error());
    }
    SL_TRACE_FUNC_CALL(logger_, res.value(), buf1, buf2);

    return getMemory().storeBuffer(res.value());
  }

}  // namespace kagome::host_api
