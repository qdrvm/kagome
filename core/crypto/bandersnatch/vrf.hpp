/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <qtils/bytes.hpp>
#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "crypto/bandersnatch_types.hpp"

namespace kagome::crypto::bandersnatch::vrf {

  using qtils::Bytes;
  using qtils::BytesIn;

  using VrfOutput = common::Blob<BANDERSNATCH_PREOUT_SIZE>;
  using RingSignature = common::Blob<BANDERSNATCH_RING_SIGNATURE_SIZE>;
  using Signature = common::Blob<BANDERSNATCH_SIGNATURE_SIZE>;

  struct VrfInput {
    VrfInput(const ::bandersnatch_VrfInput *ptr) : ptr(ptr) {}
    VrfInput(const VrfInput &) = delete;
    VrfInput(VrfInput &&x) : ptr(x.ptr) {
      const_cast<const ::bandersnatch_VrfInput *&>(x.ptr) = nullptr;
    }
    VrfInput &operator=(const VrfInput &) = delete;
    VrfInput &operator=(VrfInput &&x) = delete;
    ~VrfInput() {
      if (ptr) {
        ::bandersnatch_vrf_input_free(ptr);
      }
    }
    const ::bandersnatch_VrfInput *const ptr;
  };

  struct VrfSignData {
    VrfSignData(const ::bandersnatch_VrfSignData *ptr) : ptr(ptr) {}
    VrfSignData(const VrfSignData &) = delete;
    VrfSignData(VrfSignData &&x) : ptr(x.ptr) {
      const_cast<const ::bandersnatch_VrfSignData *&>(x.ptr) = nullptr;
    }
    VrfSignData &operator=(const VrfSignData &) = delete;
    VrfSignData &operator=(VrfSignData &&x) = delete;
    ~VrfSignData() {
      if (ptr) {
        ::bandersnatch_vrf_sign_data_free(ptr);
      }
    }
    const ::bandersnatch_VrfSignData *const ptr;
  };

  struct RingProver {
    RingProver(const ::bandersnatch_RingProver *ptr) : ptr(ptr) {}
    RingProver(const RingProver &) = delete;
    RingProver(RingProver &&x) : ptr(x.ptr) {
      const_cast<const ::bandersnatch_RingProver *&>(x.ptr) = nullptr;
    }
    RingProver &operator=(const RingProver &) = delete;
    RingProver &operator=(RingProver &&x) = delete;
    ~RingProver() {
      if (ptr) {
        ::bandersnatch_ring_prover_free(ptr);
      }
    }
    const ::bandersnatch_RingProver *const ptr;
  };

  struct RingVerifier {
    RingVerifier(const ::bandersnatch_RingVerifier *ptr) : ptr(ptr) {}
    RingVerifier(const RingVerifier &) = delete;
    RingVerifier(RingVerifier &&x) : ptr(x.ptr) {
      const_cast<const ::bandersnatch_RingVerifier *&>(x.ptr) = nullptr;
    }
    RingVerifier &operator=(const RingVerifier &) = delete;
    RingVerifier &operator=(RingVerifier &&x) = delete;
    ~RingVerifier() {
      if (ptr) {
        ::bandersnatch_ring_verifier_free(ptr);
      }
    }
    const ::bandersnatch_RingVerifier *const ptr;
  };

  /// Context used to produce ring signatures.
  class RingContext {
   public:
    static constexpr uint32_t kDomainSize = 2048;

    friend scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                                 RingContext &x) {
      static const size_t ring_context_serialized_size =
          ::bandersnatch_ring_context_serialized_size(kDomainSize);

      std::vector<uint8_t> KZG(ring_context_serialized_size);

      s >> KZG;

      x.ptr_ = ::bandersnatch_ring_vrf_context(KZG.data(), KZG.size());
      return s;
    }

    RingProver prover(std::span<const crypto::BandersnatchPublicKey> keys,
                      size_t index) const {
      std::vector<const uint8_t *> ptrs;
      std::transform(keys.begin(),
                     keys.end(),
                     std::back_inserter(ptrs),
                     [](const auto &key) { return key.data(); });
      auto prover =
          ::bandersnatch_ring_prover(ptr_, ptrs.data(), ptrs.size(), index);
      return RingProver(prover);
    }

    RingVerifier verifier(std::span<const crypto::BandersnatchPublicKey> keys,
                          size_t index) const {
      std::vector<const uint8_t *> ptrs;
      std::transform(keys.begin(),
                     keys.end(),
                     std::back_inserter(ptrs),
                     [](const auto &key) { return key.data(); });
      auto verifier =
          ::bandersnatch_ring_verifier(ptr_, ptrs.data(), ptrs.size());
      return RingVerifier(verifier);
    }

   private:
    const ::bandersnatch_RingContext *ptr_;
  };

  /// Max number of inputs/outputs which can be handled by the VRF signing
  /// procedures.
  ///
  /// The number is quite arbitrary and chosen to fulfill the use cases found so
  /// far. If required it can be extended in the future.
  constexpr size_t MAX_VRF_IOS = 3;

  template <typename T>
  concept VrfInputOrOutput =
      std::is_same_v<T, VrfInput> or std::is_same_v<T, VrfOutput>;

  template <VrfInputOrOutput T>
  using VrfIosVec = common::SLVector<T, MAX_VRF_IOS>;

  /// VRF signature.
  ///
  /// Includes both the transcript `signature` and the `outputs` generated from
  struct VrfSignature {
    SCALE_TIE(2);

    /// VRF (pre)outputs.
    VrfIosVec<VrfOutput> outputs;

    /// Transcript signature.
    Signature signature;
  };

  /// Ring VRF signature.
  struct RingVrfSignature {
    SCALE_TIE(2);

    /// VRF (pre)outputs.
    VrfIosVec<VrfOutput> outputs;

    /// Ring signature.
    RingSignature signature;
  };

  VrfInput vrf_input_from_data(BytesIn domain, std::span<BytesIn> data);

  VrfOutput vrf_output(const BandersnatchSecretKey &secret,
                       const VrfInput &input);

  template <size_t N>
    requires(N == 16 or N == 32)
  common::Blob<N> make_bytes(BytesIn context,
                             const VrfInput &input,
                             const VrfOutput &output);

  VrfSignData vrf_sign_data(BytesIn label,
                            std::span<BytesIn> data,
                            std::span<VrfInput> inputs);

  template <size_t N>
    requires(N == 16 or N == 32)
  common::Blob<N> vrf_sign_data_challenge(const VrfSignData &sign_data);

  VrfSignature vrf_sign(const BandersnatchSecretKey &secret_key,
                        const VrfSignData &sign_data);

  bool vrf_verify(const VrfSignature &signature,
                  const VrfSignData &sign_data,
                  const BandersnatchPublicKey &public_key);

  RingVrfSignature ring_vrf_sign(const BandersnatchSecretKey &secret_key,
                                 const VrfSignData &sign_data,
                                 const RingProver &ring_prover);

  bool ring_vrf_verify(const RingVrfSignature &signature,
                       const VrfSignData &sign_data,
                       const RingVerifier &verifier);

}  // namespace kagome::crypto::bandersnatch::vrf
