/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bandersnatch/bandersnatch.hpp"
#include "crypto/hasher.hpp"

namespace bandersnatch_vrfs {}

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto::bandersnatch::vrf, Error, e) {
  using E = decltype(e);
  switch (e) {
    case E::INOUT_COUNT_OVERLIMIT:
      return "Input/Output were over limit";
  }
  return "unknown error (kagome::crypto::bandersnatch::vrf::Error)";
}

namespace kagome::crypto::bandersnatch {

  SecretKey::SecretKey(const Seed &seed)  //
      : bandersnatch_vrfs::SecretKey(seed) {}

  Public SecretKey::to_public() const {
    auto pk = bandersnatch_vrfs::SecretKey::publicKey();
    return Public::fromSpan(pk).value();
  }

  vrf::VrfPreOut SecretKey::vrf_preout(const vrf::VrfInput &input) const {
    vrf::VrfPreOut vpo;
    auto po = bandersnatch_vrfs::SecretKey::vrfPreOut(input);
    BOOST_ASSERT(po.size() == vpo.size());
    std::copy(po.begin(), po.end(), vpo.begin());
    return vpo;
  }

  vrf::VrfInOut SecretKey::vrf_inout(const vrf::VrfInput &input) const {
    vrf::VrfInOut vio{};
    auto io = bandersnatch_vrfs::SecretKey::vrfInOut(input);
    BOOST_ASSERT(io.input.size() == vio.input.size());
    std::copy(io.input.begin(), io.input.end(), vio.input.begin());
    BOOST_ASSERT(io.preout.size() == vio.preoutput.size());
    std::copy(io.preout.begin(), io.preout.end(), vio.preoutput.begin());
    return vio;
  }

  /* clang-format off

#[cfg(feature = "serde")]
use crate::crypto::Ss58Codec;
use crate::crypto::{
  ByteArray, CryptoType, CryptoTypeId, Derive, Public as TraitPublic, UncheckedFrom, VrfPublic,
};
#[cfg(feature = "full_crypto")]
use crate::crypto::{DeriveError, DeriveJunction, Pair as TraitPair, SecretStringError, VrfSecret};
#[cfg(feature = "serde")]
use serde::{de, Deserialize, Deserializer, Serialize, Serializer};
#[cfg(all(not(feature = "std"), feature = "serde"))]
use sp_std::alloc::{format, string::String};

use bandersnatch_vrfs::CanonicalSerialize;
#[cfg(feature = "full_crypto")]
use bandersnatch_vrfs::SecretKey;
use codec::{Decode, Encode, EncodeLike, MaxEncodedLen};
use scale_info::TypeInfo;

use sp_runtime_interface::pass_by::PassByInner;
use sp_std::{vec, vec::Vec};

/// Identifier used to match public keys against bandersnatch-vrf keys.
pub const CRYPTO_ID: CryptoTypeId = CryptoTypeId(*b"band");

*/

  Public::Public(common::Blob<PUBLIC_SERIALIZED_SIZE> raw)
      : Blob(std::move(raw)){};

  outcome::result<Public> Public::try_from(BytesIn data) {
    OUTCOME_TRY(pub, common::Blob<Public::size()>::fromSpan(data));
    return Public{std::move(pub)};
  };

  std::optional<Public> Public::derive(
      std::span<const crypto::bip39::RawJunction> junctions) {
    return std::nullopt;  // FIXME
  }

  /* clang-format off

#[cfg(feature = "serde")]
impl Serialize for Public {
  fn serialize<S: Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
    serializer.serialize_str(&self.to_ss58check())
  }
}

#[cfg(feature = "serde")]
impl<'de> Deserialize<'de> for Public {
  fn deserialize<D: Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
    Public::from_ss58check(&String::deserialize(deserializer)?)
      .map_err(|e| de::Error::custom(format!("{:?}", e)))
  }
}

*/

  outcome::result<Pair> Pair::create(BytesIn seed_slice) {
    OUTCOME_TRY(seed, Seed::fromSpan(seed_slice));
    return Pair{Seed(std::move(seed))};
  }

  Pair::Pair(Seed seed) : secret_(seed), seed_(std::move(seed)) {}

  outcome::result<std::tuple<Pair, std::optional<Seed>>> Pair::derive(
      const Pair &original, std::span<const crypto::bip39::RawJunction> path) {
    auto derive_hard = [](auto seed, auto cc) -> Seed {
      std::shared_ptr<crypto::Hasher> hasher_;
      return hasher_->blake2b_256(
          scale::encode("bandersnatch-vrf-HDKD", seed, cc).value());
    };

    auto seed = original.seed();
    for (auto &p : path) {
      // if let DeriveJunction::Hard(cc) = p {
      //     seed = derive_hard(seed, cc);
      //   } else {
      //   return Err(DeriveError::SoftKeyInPath)
      // }
    }
    return {Pair(seed), seed};
  }

  Public Pair::publicKey() const {
    common::Blob<PUBLIC_SERIALIZED_SIZE> raw;
    // secret_.to_public().serialize_compressed(raw); // FIXME must be bind
    return Public(raw);
  }

  Signature Pair::sign(BytesIn data) const {
    std::array<BytesIn, 1> transcript_data{data};
    std::array<vrf::VrfInput, 1> i;
    auto sign_data =
        vrf::VrfSignData::create(vrf::SIGNING_CTX, transcript_data, i).value();
    return vrf_sign(sign_data).signature;
  }

  bool Pair::verify(const Signature &signature,
                    BytesIn data,
                    const Public &public_key) const {
    std::array<BytesIn, 1> transcript_data{data};
    auto sign_data =
        vrf::VrfSignData::create(vrf::SIGNING_CTX, transcript_data, {}).value();
    auto vrf_sign = vrf::VrfSignature{.signature = signature, .outputs = {}};
    return vrf::vrf_verify(sign_data, vrf_sign, public_key);
  }

  vrf::VrfSignature Pair::vrf_sign(const vrf::VrfSignData &data) const {
    BOOST_STATIC_ASSERT(vrf::kMaxVrfInputOutputCounts <= 3);
    switch (data.inputs.size()) {
      case 0:
        return vrf_sign_gen<0>(data);
      case 1:
        return vrf_sign_gen<1>(data);
      case 2:
        return vrf_sign_gen<2>(data);
      case 3:
        return vrf_sign_gen<3>(data);
    }
    BOOST_UNREACHABLE_RETURN();
  }

  vrf::VrfOutput Pair::vrf_output(const vrf::VrfInput &input) const {
    auto output = secret_.vrf_preout(input);
    return vrf::VrfOutput(output);
  }

  template <size_t N>
  vrf::VrfSignature Pair::vrf_sign_gen(const vrf::VrfSignData &data) const {
    std::vector<vrf::VrfInOut> ios;
    std::transform(data.inputs.begin(),
                   data.inputs.end(),
                   std::back_inserter(ios),
                   [&](const auto &input) { return secret_.vrf_inout(input); });

    //    auto thin_signature = bandersnatch_vrfs::ThinVrfSignature<N>(
    //        secret_.sign_thin_vrf(data.transcript, ios));

    vrf::VrfIosVec<vrf::VrfOutput> outputs;
    //    std::transform(thin_signature.preouts.begin(),
    //                   thin_signature.preouts.end(),
    //                   std::back_inserter(outputs),
    //                   [&](const auto &preout) { return VrfOutput(preout); });

    auto signature = vrf::VrfSignature{
        .signature = {},
        .outputs = std::move(outputs),
    };

    //    thin_signature.proof.serialize_compressed(signature.signature)
    //        .expect("serialization length is constant and checked by test;
    //        qed");

    return signature;
  }

  /// Generate an arbitrary number of bytes from the given `context` and VRF
  /// `input`.
  template <size_t N>
  common::Blob<N> Pair::make_bytes(BytesIn context,
                                   const vrf::VrfInput &input) {
    Transcript transcript;
    transcript.initialize(context);
    auto inout = secret_.vrf_inout(input);
    return inout.vrf_output_bytes<N>(transcript);
  }

  namespace vrf {

    VrfInput::VrfInput(BytesIn domain, BytesIn data) {
      Transcript transcript;
      transcript.initialize("TemporaryDoNotDeploy"_bytes);
      transcript.append_message("domain"_bytes, domain);
      transcript.append_message("message"_bytes, data);
      transcript.challenge_bytes("vrf-input"_bytes, *this);
    }

    template <size_t N>
    common::Blob<N> VrfInOut::vrf_output_bytes(Transcript transcript) const {
      transcript.append_message("VrfOutput"_bytes, preoutput);

      common::Blob<N> out;
      transcript.challenge_bytes(""_bytes, out);

      return out;
    }

    /* clang-format off

    /// VRF (pre)output derived from [`VrfInput`] using a [`VrfSecret`].
    ///
    /// This object is used to produce an arbitrary number of verifiable pseudo
    /// random bytes and is often called pre-output to emphasize that this is
    /// not the actual output of the VRF but an object capable of generating the
    /// output.

    struct VrfOutput : public bandersnatch_vrfs::VrfPreOut {

      impl Encode for VrfOutput {
        fn encode(&self) -> Vec<u8> {
          let mut bytes = [0; PREOUT_SERIALIZED_SIZE];
          self.0
            .serialize_compressed(bytes.as_mut_slice())
            .expect("serialization length is constant and checked by test; qed");
          bytes.encode()
        }
      }

      impl Decode for VrfOutput {
        fn decode<R: codec::Input>(i: &mut R) -> Result<Self, codec::Error> {
          let buf = <[u8; PREOUT_SERIALIZED_SIZE]>::decode(i)?;
          let preout =
            bandersnatch_vrfs::VrfPreOut::deserialize_compressed_unchecked(buf.as_slice())
              .map_err(|_| "vrf-preout decode error: bad preout")?;
          Ok(VrfOutput(preout))
        }
      }

      impl EncodeLike for VrfOutput {}

      impl MaxEncodedLen for VrfOutput {
        fn max_encoded_len() -> usize {
          <[u8; PREOUT_SERIALIZED_SIZE]>::max_encoded_len()
        }
      }

      impl TypeInfo for VrfOutput {
        type Identity = [u8; PREOUT_SERIALIZED_SIZE];

        fn type_info() -> scale_info::Type {
          Self::Identity::type_info()
        }
      }
    };
       */

    /// Generate an arbitrary number of bytes from the given `context` and VRF
    /// `input`.
    template <size_t N>
    common::Blob<N> VrfOutput::make_bytes(BytesIn context,
                                          const VrfInput &input) const {
      Transcript transcript;
      transcript.initialize(context);
      auto inout = VrfInOut{
          .input = input,
          .preoutput = {*this},
      };
      return inout.vrf_output_bytes<N>(transcript);
    }

    VrfSignData::VrfSignData(BytesIn transcript_label,
                             std::span<BytesIn> transcript_data,
                             std::span<VrfInput> inputs) {
      transcript.initialize(transcript_label);

      for (auto item : transcript_data) {
        transcript.append(item);
      }

      if (not inputs.empty()) {
        auto x = inputs.subspan(0, kMaxVrfInputOutputCounts);
        this->inputs.assign(x.begin(), x.end());
      }
    }

    outcome::result<VrfSignData> VrfSignData::create(
        BytesIn transcript_label,
        std::span<BytesIn> transcript_data,
        std::span<VrfInput> inputs) {
      if (inputs.size() > kMaxVrfInputOutputCounts) {
        return Error::INOUT_COUNT_OVERLIMIT;
      }

      return VrfSignData(transcript_label, transcript_data, inputs);
    }

    void VrfSignData::push_transcript_data(BytesIn data) {
      // transcript.append(data); // FIXME
    }

    outcome::result<void> VrfSignData::push_vrf_input(VrfInput &input) {
      if (inputs.size() < inputs.max_size()) {
        inputs.emplace_back(std::move(input));
        return outcome::success();
      }
      return Error{};
    }

    template <size_t N>
    common::Blob<N> VrfSignData::challenge() const {
      common::Blob<N> output(0, N);
      auto transcript_copy = transcript;
      transcript_copy.challenge_bytes("bandersnatch challenge"_bytes, output);
      return output;
    }

    bool vrf_verify(const VrfSignData &data,
                    const VrfSignature &signature,
                    const Public &public_key) {
      if (signature.outputs.size() != data.inputs.size()) {
        return false;
      }
      // Workaround to overcome backend signature generic over the number of IOs
      switch (signature.outputs.size()) {
        case 0:
          return vrf_verify_gen<0>(data, signature, public_key);
        case 1:
          return vrf_verify_gen<1>(data, signature, public_key);
        case 2:
          return vrf_verify_gen<2>(data, signature, public_key);
        case 3:
          return vrf_verify_gen<3>(data, signature, public_key);
      }
      BOOST_UNREACHABLE_RETURN();
    }

    template <size_t N>
    bool vrf_verify_gen(const VrfSignData &data,
                        const VrfSignature &signature,
                        const Public &public_key) {
      //  auto public_opt =
      //      bandersnatch_vrfs::PublicKey::deserialize_compressed_unchecked(
      //          as_slice());
      //  if (not public_opt.has_value()) {
      //    return false;
      //  }
      //  const auto &public_ = public_opt.value();

      std::array<VrfPreOut, N> preouts;
      std::transform(signature.outputs.begin(),
                     signature.outputs.end(),
                     preouts.begin(),
                     [](const auto &o) { return VrfPreOut(o); });

      // Deserialize only the proof, the rest has already been deserialized
      // This is another hack used because backend signature type is generic
      // over the number of ios.

      // auto sig_opt = bandersnatch_vrfs::ThinVrfSignature<
      //     0>::deserialize_compressed_unchecked(signature.signature);
      // if (not sig_opt.has_value()) {
      //   return false;
      // }
      // auto &proof = sig_opt->proof;
      //
      // auto signature_ = bandersnatch_vrfs::ThinVrfSignature{
      //     .proof = std::move(proof),
      //     .preouts = std::move(preouts),
      // };
      //
      // return public_key
      //     .verify_thin_vrf(data.transcript, data.inputs, &signature)
      //     .is_ok();

      return false;  // FIXME
    }
  }  // namespace vrf

  //
  //
  //
  //
  //
  //
  //
  //
  //
  //
  //
  //
  //
  //
  //
  //

  /* clang-format off


/// Bandersnatch Ring-VRF types and operations.
pub mod ring_vrf {
  use super::{vrf::*, *};
  pub use bandersnatch_vrfs::ring::{RingProof, RingProver, RingVerifier, KZG};
  use bandersnatch_vrfs::{ring::VerifierKey, CanonicalDeserialize, PublicKey};

  /// Overhead in the domain size with respect to the supported ring size.
  ///
  /// Some bits of the domain are reserved for the zk-proof to work.
  pub const RING_DOMAIN_OVERHEAD: u32 = 257;

  // Max size of serialized ring-vrf context given `domain_len`.
  pub(crate) const fn ring_context_serialized_size(domain_len: u32) -> usize {
    // const G1_POINT_COMPRESSED_SIZE: usize = 48;
    // const G2_POINT_COMPRESSED_SIZE: usize = 96;
    const G1_POINT_UNCOMPRESSED_SIZE: usize = 96;
    const G2_POINT_UNCOMPRESSED_SIZE: usize = 192;
    const OVERHEAD_SIZE: usize = 20;
    const G2_POINTS_NUM: usize = 2;
    let g1_points_num = 3 * domain_len as usize + 1;

    OVERHEAD_SIZE +
      g1_points_num * G1_POINT_UNCOMPRESSED_SIZE +
      G2_POINTS_NUM * G2_POINT_UNCOMPRESSED_SIZE
  }

  pub(crate) const RING_VERIFIER_DATA_SERIALIZED_SIZE: usize = 388;
  pub(crate) const RING_SIGNATURE_SERIALIZED_SIZE: usize = 755;

  /// remove as soon as soon as serialization is implemented by the backend
  pub struct RingVerifierData {
    /// Domain size.
    pub domain_size: u32,
    /// Verifier key.
    pub verifier_key: VerifierKey,
  }

  impl From<RingVerifierData> for RingVerifier {
    fn from(vd: RingVerifierData) -> RingVerifier {
      bandersnatch_vrfs::ring::make_ring_verifier(vd.verifier_key, vd.domain_size as usize)
    }
  }

  impl Encode for RingVerifierData {
    fn encode(&self) -> Vec<u8> {
      const ERR_STR: &str = "serialization length is constant and checked by test; qed";
      let mut buf = [0; RING_VERIFIER_DATA_SERIALIZED_SIZE];
      self.domain_size.serialize_compressed(&mut buf[..4]).expect(ERR_STR);
      self.verifier_key.serialize_compressed(&mut buf[4..]).expect(ERR_STR);
      buf.encode()
    }
  }

  impl Decode for RingVerifierData {
    fn decode<R: codec::Input>(i: &mut R) -> Result<Self, codec::Error> {
      const ERR_STR: &str = "serialization length is constant and checked by test; qed";
      let buf = <[u8; RING_VERIFIER_DATA_SERIALIZED_SIZE]>::decode(i)?;
      let domain_size =
        <u32 as CanonicalDeserialize>::deserialize_compressed_unchecked(&mut &buf[..4])
          .expect(ERR_STR);
      let verifier_key = <bandersnatch_vrfs::ring::VerifierKey as CanonicalDeserialize>::deserialize_compressed_unchecked(&mut &buf[4..]).expect(ERR_STR);

      Ok(RingVerifierData { domain_size, verifier_key })
    }
  }

  impl EncodeLike for RingVerifierData {}

  impl MaxEncodedLen for RingVerifierData {
    fn max_encoded_len() -> usize {
      <[u8; RING_VERIFIER_DATA_SERIALIZED_SIZE]>::max_encoded_len()
    }
  }

  impl TypeInfo for RingVerifierData {
    type Identity = [u8; RING_VERIFIER_DATA_SERIALIZED_SIZE];

    fn type_info() -> scale_info::Type {
      Self::Identity::type_info()
    }
  }

  /// Context used to construct ring prover and verifier.
  ///
  /// Generic parameter `D` represents the ring domain size and drives
  /// the max number of supported ring members [`RingContext::max_keyset_size`]
  /// which is equal to `D - RING_DOMAIN_OVERHEAD`.
  #[derive(Clone)]
  pub struct RingContext<const D: u32>(KZG);

  impl<const D: u32> RingContext<D> {
    /// Build an dummy instance for testing purposes.
    pub fn new_testing() -> Self {
      Self(KZG::testing_kzg_setup([0; 32], D))
    }

    /// Get the keyset max size.
    pub fn max_keyset_size(&self) -> usize {
      self.0.max_keyset_size()
    }

    /// Get ring prover for the key at index `public_idx` in the `public_keys` set.
    pub fn prover(&self, public_keys: &[Public], public_idx: usize) -> Option<RingProver> {
      let mut pks = Vec::with_capacity(public_keys.len());
      for public_key in public_keys {
        let pk = PublicKey::deserialize_compressed_unchecked(public_key.as_slice()).ok()?;
        pks.push(pk.0.into());
      }

      let prover_key = self.0.prover_key(pks);
      let ring_prover = self.0.init_ring_prover(prover_key, public_idx);
      Some(ring_prover)
    }

    /// Get ring verifier for the `public_keys` set.
    pub fn verifier(&self, public_keys: &[Public]) -> Option<RingVerifier> {
      let mut pks = Vec::with_capacity(public_keys.len());
      for public_key in public_keys {
        let pk = PublicKey::deserialize_compressed_unchecked(public_key.as_slice()).ok()?;
        pks.push(pk.0.into());
      }

      let verifier_key = self.0.verifier_key(pks);
      let ring_verifier = self.0.init_ring_verifier(verifier_key);
      Some(ring_verifier)
    }

    /// Information required for a lazy construction of a ring verifier.
    pub fn verifier_data(&self, public_keys: &[Public]) -> Option<RingVerifierData> {
      let mut pks = Vec::with_capacity(public_keys.len());
      for public_key in public_keys {
        let pk = PublicKey::deserialize_compressed_unchecked(public_key.as_slice()).ok()?;
        pks.push(pk.0.into());
      }
      Some(RingVerifierData {
        verifier_key: self.0.verifier_key(pks),
        domain_size: self.0.domain_size,
      })
    }
  }

  impl<const D: u32> Encode for RingContext<D> {
    fn encode(&self) -> Vec<u8> {
      let mut buf = vec![0; ring_context_serialized_size(D)];
      self.0
        .serialize_uncompressed(buf.as_mut_slice())
        .expect("serialization length is constant and checked by test; qed");
      buf
    }
  }

  impl<const D: u32> Decode for RingContext<D> {
    fn decode<R: codec::Input>(input: &mut R) -> Result<Self, codec::Error> {
      let mut buf = vec![0; ring_context_serialized_size(D)];
      input.read(&mut buf[..])?;
      let kzg = KZG::deserialize_uncompressed_unchecked(buf.as_slice())
        .map_err(|_| "KZG decode error")?;
      Ok(RingContext(kzg))
    }
  }

  impl<const D: u32> EncodeLike for RingContext<D> {}

  impl<const D: u32> MaxEncodedLen for RingContext<D> {
    fn max_encoded_len() -> usize {
      ring_context_serialized_size(D)
    }
  }

  impl<const D: u32> TypeInfo for RingContext<D> {
    type Identity = Self;

    fn type_info() -> scale_info::Type {
      let path = scale_info::Path::new("RingContext", module_path!());
      let array_type_def = scale_info::TypeDefArray {
        len: ring_context_serialized_size(D) as u32,
        type_param: scale_info::MetaType::new::<u8>(),
      };
      let type_def = scale_info::TypeDef::Array(array_type_def);
      scale_info::Type { path, type_params: Vec::new(), type_def, docs: Vec::new() }
    }
  }

  /// Ring VRF signature.
  #[derive(Clone, Debug, PartialEq, Eq, Encode, Decode, MaxEncodedLen, TypeInfo)]
  pub struct RingVrfSignature {
    /// Ring signature.
    pub signature: [u8; RING_SIGNATURE_SERIALIZED_SIZE],
    /// VRF (pre)outputs.
    pub outputs: VrfIosVec<VrfOutput>,
  }

  #[cfg(feature = "full_crypto")]
  impl Pair {
    /// Produce a ring-vrf signature.
    ///
    /// The ring signature is verifiable if the public key corresponding to the
    /// signing [`Pair`] is part of the ring from which the [`RingProver`] has
    /// been constructed. If not, the produced signature is just useless.
    pub fn ring_vrf_sign(&self, data: &VrfSignData, prover: &RingProver) -> RingVrfSignature {
      const _: () = assert!(MAX_VRF_IOS == 3, "`MAX_VRF_IOS` expected to be 3");
      // Workaround to overcome backend signature generic over the number of IOs.
      match data.inputs.len() {
        0 => self.ring_vrf_sign_gen::<0>(data, prover),
        1 => self.ring_vrf_sign_gen::<1>(data, prover),
        2 => self.ring_vrf_sign_gen::<2>(data, prover),
        3 => self.ring_vrf_sign_gen::<3>(data, prover),
        _ => unreachable!(),
      }
    }

    fn ring_vrf_sign_gen<const N: usize>(
      &self,
      data: &VrfSignData,
      prover: &RingProver,
    ) -> RingVrfSignature {
      let ios = core::array::from_fn(|i| self.secret.vrf_inout(data.inputs[i].0));

      let ring_signature: bandersnatch_vrfs::RingVrfSignature<N> =
        bandersnatch_vrfs::RingProver { ring_prover: prover, secret: &self.secret }
          .sign_ring_vrf(data.transcript.clone(), &ios);

      let outputs: Vec<_> = ring_signature.preouts.into_iter().map(VrfOutput).collect();
      let outputs = VrfIosVec::truncate_from(outputs);

      let mut signature =
        RingVrfSignature { outputs, signature: [0; RING_SIGNATURE_SERIALIZED_SIZE] };

      ring_signature
        .proof
        .serialize_compressed(signature.signature.as_mut_slice())
        .expect("serialization length is constant and checked by test; qed");

      signature
    }
  }

  impl RingVrfSignature {
    /// Verify a ring-vrf signature.
    ///
    /// The signature is verifiable if it has been produced by a member of the ring
    /// from which the [`RingVerifier`] has been constructed.
    pub fn ring_vrf_verify(&self, data: &VrfSignData, verifier: &RingVerifier) -> bool {
      const _: () = assert!(MAX_VRF_IOS == 3, "`MAX_VRF_IOS` expected to be 3");
      let preouts_len = self.outputs.len();
      if preouts_len != data.inputs.len() {
        return false
      }
      // Workaround to overcome backend signature generic over the number of IOs.
      match preouts_len {
        0 => self.ring_vrf_verify_gen::<0>(data, verifier),
        1 => self.ring_vrf_verify_gen::<1>(data, verifier),
        2 => self.ring_vrf_verify_gen::<2>(data, verifier),
        3 => self.ring_vrf_verify_gen::<3>(data, verifier),
        _ => unreachable!(),
      }
    }

    fn ring_vrf_verify_gen<const N: usize>(
      &self,
      data: &VrfSignData,
      verifier: &RingVerifier,
    ) -> bool {
      let Ok(vrf_signature) =
        bandersnatch_vrfs::RingVrfSignature::<0>::deserialize_compressed_unchecked(
          self.signature.as_slice(),
        )
      else {
        return false
      };

      let preouts: [bandersnatch_vrfs::VrfPreOut; N] =
        core::array::from_fn(|i| self.outputs[i].0);

      let signature =
        bandersnatch_vrfs::RingVrfSignature { proof: vrf_signature.proof, preouts };

      let inputs = data.inputs.iter().map(|i| i.0);

      bandersnatch_vrfs::RingVerifier(verifier)
        .verify_ring_vrf(data.transcript.clone(), inputs, &signature)
        .is_ok()
    }
  }
}

#[cfg(test)]
mod tests {
  use super::{ring_vrf::*, vrf::*, *};
  use crate::crypto::{VrfPublic, VrfSecret, DEV_PHRASE};

  const DEV_SEED: &[u8; SEED_SERIALIZED_SIZE] = &[0xcb; SEED_SERIALIZED_SIZE];
  const TEST_DOMAIN_SIZE: u32 = 1024;

  type TestRingContext = RingContext<TEST_DOMAIN_SIZE>;

  #[allow(unused)]
  fn b2h(bytes: &[u8]) -> String {
    array_bytes::bytes2hex("", bytes)
  }

  fn h2b(hex: &str) -> Vec<u8> {
    array_bytes::hex2bytes_unchecked(hex)
  }

  #[test]
  fn backend_assumptions_sanity_check() {
    let kzg = KZG::testing_kzg_setup([0; 32], TEST_DOMAIN_SIZE);
    assert_eq!(kzg.max_keyset_size() as u32, TEST_DOMAIN_SIZE - RING_DOMAIN_OVERHEAD);

    assert_eq!(kzg.uncompressed_size(), ring_context_serialized_size(TEST_DOMAIN_SIZE));

    let pks: Vec<_> = (0..16)
      .map(|i| SecretKey::from_seed(&[i as u8; 32]).to_public().0.into())
      .collect();

    let secret = SecretKey::from_seed(&[0u8; 32]);

    let public = secret.to_public();
    assert_eq!(public.compressed_size(), PUBLIC_SERIALIZED_SIZE);

    let input = VrfInput::new(b"foo", &[]);
    let preout = secret.vrf_preout(&input.0);
    assert_eq!(preout.compressed_size(), PREOUT_SERIALIZED_SIZE);

    let verifier_key = kzg.verifier_key(pks.clone());
    assert_eq!(verifier_key.compressed_size() + 4, RING_VERIFIER_DATA_SERIALIZED_SIZE);

    let prover_key = kzg.prover_key(pks);
    let ring_prover = kzg.init_ring_prover(prover_key, 0);

    let data = VrfSignData::new_unchecked(b"mydata", &[b"tdata"], None);

    let thin_signature: bandersnatch_vrfs::ThinVrfSignature<0> =
      secret.sign_thin_vrf(data.transcript.clone(), &[]);
    assert_eq!(thin_signature.compressed_size(), SIGNATURE_SERIALIZED_SIZE);

    let ring_signature: bandersnatch_vrfs::RingVrfSignature<0> =
      bandersnatch_vrfs::RingProver { ring_prover: &ring_prover, secret: &secret }
        .sign_ring_vrf(data.transcript.clone(), &[]);
    assert_eq!(ring_signature.compressed_size(), RING_SIGNATURE_SERIALIZED_SIZE);
  }

  #[test]
  fn max_vrf_ios_bound_respected() {
    let inputs: Vec<_> = (0..MAX_VRF_IOS - 1).map(|_| VrfInput::new(b"", &[])).collect();
    let mut sign_data = VrfSignData::new(b"", &[b""], inputs).unwrap();
    let res = sign_data.push_vrf_input(VrfInput::new(b"", b""));
    assert!(res.is_ok());
    let res = sign_data.push_vrf_input(VrfInput::new(b"", b""));
    assert!(res.is_err());
    let inputs: Vec<_> = (0..MAX_VRF_IOS + 1).map(|_| VrfInput::new(b"", b"")).collect();
    let res = VrfSignData::new(b"mydata", &[b"tdata"], inputs);
    assert!(res.is_err());
  }

  #[test]
  fn derive_works() {
    let pair = Pair::from_string(&format!("{}//Alice//Hard", DEV_PHRASE), None).unwrap();
    let known = h2b("2b340c18b94dc1916979cb83daf3ed4ac106742ddc06afc42cf26be3b18a523f80");
    assert_eq!(pair.public().as_ref(), known);

    // Soft derivation not supported
    let res = Pair::from_string(&format!("{}//Alice/Soft", DEV_PHRASE), None);
    assert!(res.is_err());
  }

  #[test]
  fn sign_verify() {
    let pair = Pair::from_seed(DEV_SEED);
    let public = pair.public();
    let msg = b"hello";

    let signature = pair.sign(msg);
    assert!(Pair::verify(&signature, msg, &public));
  }

  #[test]
  fn vrf_sign_verify() {
    let pair = Pair::from_seed(DEV_SEED);
    let public = pair.public();

    let i1 = VrfInput::new(b"dom1", b"foo");
    let i2 = VrfInput::new(b"dom2", b"bar");
    let i3 = VrfInput::new(b"dom3", b"baz");

    let data = VrfSignData::new_unchecked(b"mydata", &[b"tdata"], [i1, i2, i3]);

    let signature = pair.vrf_sign(&data);

    assert!(public.vrf_verify(&data, &signature));
  }

  #[test]
  fn vrf_sign_verify_bad_inputs() {
    let pair = Pair::from_seed(DEV_SEED);
    let public = pair.public();

    let i1 = VrfInput::new(b"dom1", b"foo");
    let i2 = VrfInput::new(b"dom2", b"bar");

    let data = VrfSignData::new_unchecked(b"mydata", &[b"aaaa"], [i1.clone(), i2.clone()]);
    let signature = pair.vrf_sign(&data);

    let data = VrfSignData::new_unchecked(b"mydata", &[b"bbb"], [i1, i2.clone()]);
    assert!(!public.vrf_verify(&data, &signature));

    let data = VrfSignData::new_unchecked(b"mydata", &[b"aaa"], [i2]);
    assert!(!public.vrf_verify(&data, &signature));
  }

  #[test]
  fn vrf_make_bytes_matches() {
    let pair = Pair::from_seed(DEV_SEED);

    let i1 = VrfInput::new(b"dom1", b"foo");
    let i2 = VrfInput::new(b"dom2", b"bar");

    let data = VrfSignData::new_unchecked(b"mydata", &[b"tdata"], [i1.clone(), i2.clone()]);
    let signature = pair.vrf_sign(&data);

    let o10 = pair.make_bytes::<32>(b"ctx1", &i1);
    let o11 = signature.outputs[0].make_bytes::<32>(b"ctx1", &i1);
    assert_eq!(o10, o11);

    let o20 = pair.make_bytes::<48>(b"ctx2", &i2);
    let o21 = signature.outputs[1].make_bytes::<48>(b"ctx2", &i2);
    assert_eq!(o20, o21);
  }

  #[test]
  fn encode_decode_vrf_signature() {
    // Transcript data is hashed together and signed.
    // It doesn't contribute to serialized length.
    let pair = Pair::from_seed(DEV_SEED);

    let i1 = VrfInput::new(b"dom1", b"foo");
    let i2 = VrfInput::new(b"dom2", b"bar");

    let data = VrfSignData::new_unchecked(b"mydata", &[b"tdata"], [i1.clone(), i2.clone()]);
    let expected = pair.vrf_sign(&data);

    let bytes = expected.encode();

    let expected_len =
      data.inputs.len() * PREOUT_SERIALIZED_SIZE + SIGNATURE_SERIALIZED_SIZE + 1;
    assert_eq!(bytes.len(), expected_len);

    let decoded = VrfSignature::decode(&mut bytes.as_slice()).unwrap();
    assert_eq!(expected, decoded);

    let data = VrfSignData::new_unchecked(b"mydata", &[b"tdata"], []);
    let expected = pair.vrf_sign(&data);

    let bytes = expected.encode();

    let decoded = VrfSignature::decode(&mut bytes.as_slice()).unwrap();
    assert_eq!(expected, decoded);
  }

  #[test]
  fn ring_vrf_sign_verify() {
    let ring_ctx = TestRingContext::new_testing();

    let mut pks: Vec<_> = (0..16).map(|i| Pair::from_seed(&[i as u8; 32]).public()).collect();
    assert!(pks.len() <= ring_ctx.max_keyset_size());

    let pair = Pair::from_seed(DEV_SEED);

    // Just pick one index to patch with the actual public key
    let prover_idx = 3;
    pks[prover_idx] = pair.public();

    let i1 = VrfInput::new(b"dom1", b"foo");
    let i2 = VrfInput::new(b"dom2", b"bar");
    let i3 = VrfInput::new(b"dom3", b"baz");

    let data = VrfSignData::new_unchecked(b"mydata", &[b"tdata"], [i1, i2, i3]);

    let prover = ring_ctx.prover(&pks, prover_idx).unwrap();
    let signature = pair.ring_vrf_sign(&data, &prover);

    let verifier = ring_ctx.verifier(&pks).unwrap();
    assert!(signature.ring_vrf_verify(&data, &verifier));
  }

  #[test]
  fn ring_vrf_sign_verify_with_out_of_ring_key() {
    let ring_ctx = TestRingContext::new_testing();

    let pks: Vec<_> = (0..16).map(|i| Pair::from_seed(&[i as u8; 32]).public()).collect();
    let pair = Pair::from_seed(DEV_SEED);

    // Just pick one index to patch with the actual public key
    let i1 = VrfInput::new(b"dom1", b"foo");
    let data = VrfSignData::new_unchecked(b"mydata", Some(b"tdata"), Some(i1));

    // pair.public != pks[0]
    let prover = ring_ctx.prover(&pks, 0).unwrap();
    let signature = pair.ring_vrf_sign(&data, &prover);

    let verifier = ring_ctx.verifier(&pks).unwrap();
    assert!(!signature.ring_vrf_verify(&data, &verifier));
  }

  #[test]
  fn ring_vrf_make_bytes_matches() {
    let ring_ctx = TestRingContext::new_testing();

    let mut pks: Vec<_> = (0..16).map(|i| Pair::from_seed(&[i as u8; 32]).public()).collect();
    assert!(pks.len() <= ring_ctx.max_keyset_size());

    let pair = Pair::from_seed(DEV_SEED);

    // Just pick one index to patch with the actual public key
    let prover_idx = 3;
    pks[prover_idx] = pair.public();

    let i1 = VrfInput::new(b"dom1", b"foo");
    let i2 = VrfInput::new(b"dom2", b"bar");
    let data = VrfSignData::new_unchecked(b"mydata", &[b"tdata"], [i1.clone(), i2.clone()]);

    let prover = ring_ctx.prover(&pks, prover_idx).unwrap();
    let signature = pair.ring_vrf_sign(&data, &prover);

    let o10 = pair.make_bytes::<32>(b"ctx1", &i1);
    let o11 = signature.outputs[0].make_bytes::<32>(b"ctx1", &i1);
    assert_eq!(o10, o11);

    let o20 = pair.make_bytes::<48>(b"ctx2", &i2);
    let o21 = signature.outputs[1].make_bytes::<48>(b"ctx2", &i2);
    assert_eq!(o20, o21);
  }

  #[test]
  fn encode_decode_ring_vrf_signature() {
    let ring_ctx = TestRingContext::new_testing();

    let mut pks: Vec<_> = (0..16).map(|i| Pair::from_seed(&[i as u8; 32]).public()).collect();
    assert!(pks.len() <= ring_ctx.max_keyset_size());

    let pair = Pair::from_seed(DEV_SEED);

    // Just pick one...
    let prover_idx = 3;
    pks[prover_idx] = pair.public();

    let i1 = VrfInput::new(b"dom1", b"foo");
    let i2 = VrfInput::new(b"dom2", b"bar");
    let i3 = VrfInput::new(b"dom3", b"baz");

    let data = VrfSignData::new_unchecked(b"mydata", &[b"tdata"], [i1, i2, i3]);

    let prover = ring_ctx.prover(&pks, prover_idx).unwrap();
    let expected = pair.ring_vrf_sign(&data, &prover);

    let bytes = expected.encode();

    let expected_len =
      data.inputs.len() * PREOUT_SERIALIZED_SIZE + RING_SIGNATURE_SERIALIZED_SIZE + 1;
    assert_eq!(bytes.len(), expected_len);

    let decoded = RingVrfSignature::decode(&mut bytes.as_slice()).unwrap();
    assert_eq!(expected, decoded);
  }

  #[test]
  fn encode_decode_ring_vrf_context() {
    let ctx1 = TestRingContext::new_testing();
    let enc1 = ctx1.encode();

    let _ti = <TestRingContext as TypeInfo>::type_info();

    assert_eq!(enc1.len(), ring_context_serialized_size(TEST_DOMAIN_SIZE));
    assert_eq!(enc1.len(), TestRingContext::max_encoded_len());

    let ctx2 = TestRingContext::decode(&mut enc1.as_slice()).unwrap();
    let enc2 = ctx2.encode();

    assert_eq!(enc1, enc2);
  }

  #[test]
  fn encode_decode_verifier_data() {
    let ring_ctx = TestRingContext::new_testing();

    let pks: Vec<_> = (0..16).map(|i| Pair::from_seed(&[i as u8; 32]).public()).collect();
    assert!(pks.len() <= ring_ctx.max_keyset_size());

    let verifier_data = ring_ctx.verifier_data(&pks).unwrap();
    let enc1 = verifier_data.encode();

    assert_eq!(enc1.len(), RING_VERIFIER_DATA_SERIALIZED_SIZE);
    assert_eq!(RingVerifierData::max_encoded_len(), RING_VERIFIER_DATA_SERIALIZED_SIZE);

    let vd2 = RingVerifierData::decode(&mut enc1.as_slice()).unwrap();
    let enc2 = vd2.encode();

    assert_eq!(enc1, enc2);
  }
}

*/

}  // namespace kagome::crypto::bandersnatch
