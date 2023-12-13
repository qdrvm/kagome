/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <ranges>

#include "common/blob.hpp"
#include "common/buffer_view.hpp"
#include "common/size_limited_containers.hpp"
#include "crypto/bip39/bip39_types.hpp"
#include "primitives/transcript.hpp"

#include <libp2p/common/types.hpp>

#include <bandersnatch_vrfs/bandersnatch_vrfs.hpp>

namespace kagome::crypto::bandersnatch {

  using libp2p::Bytes;
  using libp2p::BytesIn;
  using libp2p::BytesOut;

  using Transcript = primitives::Transcript;  // bandersnatch_vrfs::Transcript

  const size_t SEED_SERIALIZED_SIZE = 32;
  const size_t PUBLIC_SERIALIZED_SIZE = 33;
  const size_t SIGNATURE_SERIALIZED_SIZE = 65;
  const size_t PREOUT_SERIALIZED_SIZE = 33;

  // Forward declarations
  struct Seed;
  struct SecretKey;
  struct Public;
  struct Pair;
  struct Signature;
  namespace vrf {
    struct VrfInput;
    struct VrfOutput;
    struct VrfPreOut;
    struct VrfInOut;
    struct VrfSignData;
    struct VrfSignature;
  }  // namespace vrf

  /// The raw secret seed, which can be used to reconstruct the secret [`Pair`].
  struct Seed : public common::Blob<SEED_SERIALIZED_SIZE> {
    using common::Blob<SEED_SERIALIZED_SIZE>::Blob;
    //
  };

  struct SecretKey : private bandersnatch_vrfs::SecretKey {
    explicit SecretKey(const Seed &seed);

    Public to_public() const;

    vrf::VrfPreOut vrf_preout(const vrf::VrfInput &input) const;

    vrf::VrfInOut vrf_inout(const vrf::VrfInput &input) const;
  };

  /// Bandersnatch public key.
  struct Public : public common::Blob<PUBLIC_SERIALIZED_SIZE> {
    Public(common::Blob<PUBLIC_SERIALIZED_SIZE> raw);

    static outcome::result<Public> try_from(BytesIn data);

    std::optional<Public> derive(
        std::span<const crypto::bip39::RawJunction> junctions);
  };

  /// Bandersnatch secret key.
  struct Pair {
    /// Generate new key pair from the provided `seed`.
    ///
    /// @WARNING: THIS WILL ONLY BE SECURE IF THE `seed` IS SECURE. If it can be
    /// guessed by an attacker then they can also derive your key.
    Pair(Seed seed);

    /// Make a new key pair from secret seed material.
    ///
    /// The slice must be 32 bytes long or it will return an error.
    outcome::result<Pair> create(BytesIn seed_slice);

    const Seed &seed() const {
      return seed_;
    }

    Public publicKey() const;

    /// Derive a child key from a series of given (hard) junctions.
    ///
    /// Soft junctions are not supported.
    static outcome::result<std::tuple<Pair, std::optional<Seed>>> derive(
        const Pair &original, std::span<const crypto::bip39::RawJunction> path);

    /// Sign a message.
    ///
    /// In practice this produce a Schnorr signature of a transcript composed by
    /// the constant label [`SIGNING_CTX`] and `data` without any additional
    /// data.
    ///
    /// See [`vrf::VrfSignData`] for additional details.
    Signature sign(BytesIn data) const;

    bool verify(const Signature &signature,
                BytesIn data,
                const Public &public_key) const;

   private:
    vrf::VrfSignature vrf_sign(const vrf::VrfSignData &data) const;

    template <size_t N>
    vrf::VrfSignature vrf_sign_gen(const vrf::VrfSignData &data) const;

    vrf::VrfOutput vrf_output(const vrf::VrfInput &input) const;

    /// Generate an arbitrary number of bytes from the given `context` and VRF
    /// `input`.
    template <size_t N>
    common::Blob<N> make_bytes(BytesIn context, const vrf::VrfInput &input);

    SecretKey secret_;
    Seed seed_;
  };

  /// Bandersnatch signature.
  ///
  /// The signature is created via the [`VrfSecret::vrf_sign`] using
  /// [`SIGNING_CTX`] as transcript `label`.
  struct Signature : public common::Blob<SIGNATURE_SERIALIZED_SIZE> {
    static const auto LEN = size();

    static Signature unchecked_from(
        common::Blob<SIGNATURE_SERIALIZED_SIZE> raw) {
      return Signature{std::move(raw)};
    };

    static outcome::result<Signature> try_from(std::span<uint8_t> data) {
      OUTCOME_TRY(pub, common::Blob<SIGNATURE_SERIALIZED_SIZE>::fromSpan(data));
      return Signature{std::move(pub)};
    };

    using Pair = Pair;
  };

  /// Bandersnatch VRF types and operations.
  namespace vrf {

    enum class Error {
      INOUT_COUNT_OVERLIMIT = 1,
    };

    using AffineRepr = common::Blob<33>;

    struct Message {
      BytesIn domain;
      BytesIn message;
    };

    /// VRF input to construct a [`VrfOutput`] instance and embeddable in
    /// [`VrfSignData`].
    struct VrfInput : public AffineRepr  // public bandersnatch_vrfs::VrfInput
    {
      VrfInput() = default;

      /// Construct a new VRF input.
      VrfInput(BytesIn domain, BytesIn data);
    };

    struct VrfPreOut : public AffineRepr  // public bandersnatch_vrfs::VrfInput
    {};

    struct VrfInOut {
      /// VRF input point
      VrfInput input{};
      /// VRF pre-output point
      VrfPreOut preoutput{};

      template <size_t N>
      common::Blob<N> vrf_output_bytes(Transcript transcript) const;
    };

    /// VRF (pre)output derived from [`VrfInput`] using a [`VrfSecret`].
    ///
    /// This object is used to produce an arbitrary number of verifiable
    /// pseudo random bytes and is often called pre-output to emphasize that
    /// this is not the actual output of the VRF but an object capable of
    /// generating the output.
    struct VrfOutput : AffineRepr  // public bandersnatch_vrfs::VrfPreOut
    {
      VrfOutput(VrfPreOut preout) : AffineRepr(preout) {}

      /// Generate an arbitrary number of bytes from the given `context` and
      /// VRF `input`.
      template <size_t N>
      common::Blob<N> make_bytes(BytesIn context, const VrfInput &input) const;
    };

    /// Max number of inputs/outputs which can be handled by the VRF signing
    /// procedures.
    ///
    /// The number is quite arbitrary and chosen to fulfill the use cases
    /// found so far. If required it can be extended in the future.
    static constexpr size_t kMaxVrfInputOutputCounts = 3;

    /// Bounded vector used for VRF inputs and outputs.
    ///
    /// Can contain at most [`MAX_VRF_IOS`] elements.
    template <typename T>
      requires std::is_same_v<T, VrfInput> or std::is_same_v<T, VrfOutput>
    using VrfIosVec = common::SLVector<T, kMaxVrfInputOutputCounts>;

    /// Context used to produce a plain signature without any VRF
    /// input/output.
    const auto SIGNING_CTX = "BandersnatchSigningContext"_bytes;

    /// Data to be signed via one of the two provided vrf flavors.
    ///
    /// The object contains a transcript and a sequence of [`VrfInput`]s ready
    /// to be signed.
    ///
    /// The `transcript` summarizes a set of messages which are defining a
    /// particular protocol by automating the Fiat-Shamir transform for
    /// challenge generation. A good explaination of the topic can be found in
    /// Merlin [docs](https://merlin.cool/)
    ///
    /// The `inputs` is a sequence of [`VrfInput`]s which, during the signing
    /// procedure, are first transformed to [`VrfOutput`]s. Both inputs and
    /// outputs are then appended to the transcript before signing the
    /// Fiat-Shamir transform result (the challenge).
    ///
    /// In practice, as a user, all these technical details can be easily
    /// ignored. What is important to remember is:
    /// - *Transcript* is an object defining the protocol and used to produce
    /// the signature. This object doesn't influence the `VrfOutput`s values.
    /// - *Vrf inputs* is some additional data which is used to produce *vrf
    /// outputs*. This data will contribute to the signature as well.
    struct VrfSignData {
      /// Associated protocol transcript.
      Transcript transcript;
      /// VRF inputs to be signed.
      VrfIosVec<VrfInput> inputs;

      /// Construct a new data to be signed.
      ///
      /// At most the first [`MAX_VRF_IOS`] elements of `inputs` are used.
      ///
      /// Refer to [`VrfSignData`] for details about transcript and inputs.
      VrfSignData(BytesIn transcript_label,
                  std::span<BytesIn> transcript_data,
                  std::span<VrfInput> inputs);

      /// Construct a new data to be signed.
      ///
      /// Fails if the `inputs` iterator yields more elements than
      /// [`MAX_VRF_IOS`]
      ///
      /// Refer to [`VrfSignData`] for details about transcript and inputs.
      static outcome::result<VrfSignData> create(
          BytesIn transcript_label,
          std::span<BytesIn> transcript_data,
          std::span<VrfInput> inputs);

      /// Append a message to the transcript.
      void push_transcript_data(BytesIn data);

      /// Tries to append a [`VrfInput`] to the vrf inputs list.
      ///
      /// On failure, input parameter is not changed.
      outcome::result<void> push_vrf_input(VrfInput &input);

      /// Get the challenge associated to the `transcript` contained within
      /// the signing data.
      ///
      /// Ignores the vrf inputs and outputs.
      template <size_t N>
      common::Blob<N> challenge() const;
    };

    /// VRF signature.
    ///
    /// Includes both the transcript `signature` and the `outputs` generated
    /// from the [`VrfSignData::inputs`].
    ///
    /// Refer to [`VrfSignData`] for more details.
    struct VrfSignature {
      /// Transcript signature.
      Signature signature;
      /// VRF (pre)outputs.
      VrfIosVec<VrfOutput> outputs;
    };

    bool vrf_verify(const VrfSignData &data,
                    const VrfSignature &signature,
                    const Public &public_key);

    template <size_t N>
    bool vrf_verify_gen(const VrfSignData &data,
                        const VrfSignature &signature,
                        const Public &public_key);

  }  // namespace vrf

}  // namespace kagome::crypto::bandersnatch

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto::bandersnatch::vrf, Error);
