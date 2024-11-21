/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KAGOME_SCALE_HPP
#define KAGOME_KAGOME_SCALE_HPP

#include <span>
#include <type_traits>
#include "common/blob.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "crypto/ecdsa_types.hpp"
#include "consensus/babe/types/seal.hpp"
#include "network/types/blocks_response.hpp"
#include "network/types/roles.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "primitives/justification.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "scale/big_fixed_integers.hpp"
#include "scale/encode_append.hpp"
#include "scale/libp2p_types.hpp"
#include "consensus/beefy/types.hpp"
#include "consensus/grandpa/types/equivocation_proof.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "network/types/dispute_messages.hpp"

namespace kagome::scale {
  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockHeader &bh) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const consensus::grandpa::Equivocation &bh) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockHeaderReflection &bh) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockReflection &bh) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const network::BlocksResponse &b) requires std::is_invocable_v<F, const uint8_t *const, size_t>;
  
  //template <typename F>
  //constexpr void encode(const F &func, const kagome::consensus::beefy::SignedCommitment &b) requires std::is_invocable_v<F, const uint8_t *const, size_t>;


  template <typename F>
  constexpr void encode(const F &func, const kagome::network::vstaging::CompactStatement &c)  requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F,
            typename ElementType,
            size_t MaxSize,
            typename Allocator>
  constexpr void encode(
      const F &func,
      const common::SLVector<ElementType, MaxSize, Allocator> &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F, typename T, typename Tag, typename Base>
  constexpr void encode(const F &func, const Tagged<T, Tag, Base> &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F, size_t MaxSize>
  constexpr void encode(const F &func, const common::SLBuffer<MaxSize> &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const network::Roles &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const primitives::Other &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const primitives::Consensus &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func,
                        const runtime::PersistedValidationData &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const primitives::Seal &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const primitives::PreRuntime &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockInfo &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func,
                        const primitives::RuntimeEnvironmentUpdated &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  void encode(const F &func, const crypto::EcdsaSignature &value) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  void encode(const F &func, const crypto::EcdsaPublicKey &value) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const ::scale::EncodeOpaqueValue &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func,
                        const consensus::babe::BabeBlockHeader &bh) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const kagome::network::CandidateCommitments &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const kagome::network::CandidateReceipt &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const kagome::network::InvalidDisputeVote &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const kagome::network::ValidDisputeVote &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;

  template <typename F>
  constexpr void encode(const F &func, const consensus::grandpa::SignedPrecommit &c) requires std::is_invocable_v<F, const uint8_t *const, size_t>;
  
}  // namespace kagome::scale

#include "scale/encoder/primitives.hpp"

namespace kagome::scale {

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockHeader &bh)  requires std::is_invocable_v<F, const uint8_t *const, size_t>{
    kagome::scale::encode(func, bh.parent_hash);
    kagome::scale::encodeCompact(func, bh.number);
    kagome::scale::encode(func, bh.state_root);
    kagome::scale::encode(func, bh.extrinsics_root);
    kagome::scale::encode(func, bh.digest);
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockReflection &bh)  requires std::is_invocable_v<F, const uint8_t *const, size_t>{
    kagome::scale::encode(func, bh.header);
    kagome::scale::encode(func, bh.body);
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockHeaderReflection &bh)  requires std::is_invocable_v<F, const uint8_t *const, size_t>{
    kagome::scale::encode(func, bh.parent_hash);
    kagome::scale::encodeCompact(func, bh.number);
    kagome::scale::encode(func, bh.state_root);
    kagome::scale::encode(func, bh.extrinsics_root);
    kagome::scale::encode(func, bh.digest);
  }

  template <typename F>
  constexpr void encode(const F &func, const network::BlocksResponse &b) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, b.blocks);
  }

//  template <typename F>
//  constexpr void encode(const F &func, const kagome::network::vstaging::AttestedCandidateRequest &b) {
//    encode(func, b.candidate_hash);
//    encode(func, b.mask);
//  }

  template <typename F>
  constexpr void encode(const F &func,
                        const consensus::babe::BabeBlockHeader &bh)  requires std::is_invocable_v<F, const uint8_t *const, size_t>{
    kagome::scale::encode(func, bh.slot_assignment_type);
    kagome::scale::encode(func, bh.authority_index);
    kagome::scale::encode(func, bh.slot_number);

    if (bh.needVRFCheck()) {
      kagome::scale::encode(func, bh.vrf_output);
    }
  }

  template <typename F,
            typename ElementType,
            size_t MaxSize,
            typename Allocator>
  constexpr void encode(
      const F &func,
      const common::SLVector<ElementType, MaxSize, Allocator> &c)  requires std::is_invocable_v<F, const uint8_t *const, size_t>{
    kagome::scale::encode(func, static_cast<const std::vector<ElementType, Allocator> &>(c));
  }

  template <typename F, typename T, typename Tag, typename Base>
  constexpr void encode(const F &func, const Tagged<T, Tag, Base> &c)  requires std::is_invocable_v<F, const uint8_t *const, size_t>{
    if constexpr (std::is_scalar_v<T>) {
      kagome::scale::encode(func, c.Wrapper<T>::value);
    } else {
      kagome::scale::encode(func, static_cast<const T &>(c));
    }
  }

  //template <typename F>
  //constexpr void encode(const F &func, const kagome::consensus::beefy::SignedCommitment &b) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
  //  kagome::scale::encode(func, b.commitment);
  //  kagome::scale::encode(func, b.signatures);
  //}

  template <typename F, size_t MaxSize>
  constexpr void encode(const F &func, const common::SLBuffer<MaxSize> &c) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, static_cast<const common::SLVector<uint8_t, MaxSize> &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::Other &c)  requires std::is_invocable_v<F, const uint8_t *const, size_t>{
    kagome::scale::encode(func, static_cast<const common::Buffer &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::Consensus &c) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func,
                        const kagome::runtime::PersistedValidationData &c) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, c.parent_head);
    kagome::scale::encode(func, c.relay_parent_number);
    kagome::scale::encode(func, c.relay_parent_storage_root);
    kagome::scale::encode(func, c.max_pov_size);
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::Seal &c) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::PreRuntime &c) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockInfo &c)  requires std::is_invocable_v<F, const uint8_t *const, size_t>{
    kagome::scale::encode(func, c.number);
    kagome::scale::encode(func, c.hash);
  }

  template <typename F>
  constexpr void encode(const F &func,
                        const primitives::RuntimeEnvironmentUpdated &c)  requires std::is_invocable_v<F, const uint8_t *const, size_t>{}

  template <typename F>
  constexpr void encode(const F &func, const ::scale::EncodeOpaqueValue &c) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    putByte(func, c.v.data(), c.v.size());
  }

  template <typename F>
  constexpr void encode(const F &func, const network::Roles &c)  requires std::is_invocable_v<F, const uint8_t *const, size_t>{
    kagome::scale::encode(func, c.value);
  }

  template <typename F>
  constexpr void encode(const F &func, const consensus::grandpa::Equivocation &bh) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, bh.stage);
    kagome::scale::encode(func, bh.round_number);
    kagome::scale::encode(func, bh.first);
    kagome::scale::encode(func, bh.second);
  }

  template <typename F>
  constexpr void encode(const F &func, const kagome::network::CandidateCommitments &c) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, c.upward_msgs);
    kagome::scale::encode(func, c.outbound_hor_msgs);
    kagome::scale::encode(func, c.opt_para_runtime);
    kagome::scale::encode(func, c.para_head);
    kagome::scale::encode(func, c.downward_msgs_count);
    kagome::scale::encode(func, c.watermark);
  }

  template <typename F>
  constexpr void encode(const F &func, const kagome::network::CandidateReceipt &c) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, c.descriptor);
    kagome::scale::encode(func, c.commitments_hash);
  }

  template <typename F>
  constexpr void encode(const F &func, const kagome::network::vstaging::CompactStatement &c)  requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, c.header);
    kagome::scale::encode(func, c.inner_value);
  }

  template <typename F>
  constexpr void encode(const F &func, const kagome::network::InvalidDisputeVote &c) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, c.index);
    kagome::scale::encode(func, c.signature);
    kagome::scale::encode(func, c.kind);
  }

  template <typename F>
  constexpr void encode(const F &func, const kagome::network::ValidDisputeVote &c) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, c.index);
    kagome::scale::encode(func, c.signature);
    kagome::scale::encode(func, c.kind);
  }

  template <typename F>
  constexpr void encode(const F &func, const consensus::grandpa::SignedPrecommit &c) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, static_cast<const consensus::grandpa::SignedMessage &>(c));
  }

  template <typename F>
  void encode(const F &func, const crypto::EcdsaSignature &data) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, static_cast<const common::Blob<crypto::constants::ecdsa::SIGNATURE_SIZE> &>(data));
  }

  template <typename F>
  void encode(const F &func, const crypto::EcdsaPublicKey &data) requires std::is_invocable_v<F, const uint8_t *const, size_t> {
    kagome::scale::encode(func, static_cast<const common::Blob<crypto::constants::ecdsa::PUBKEY_SIZE> &>(data));
  }
}  // namespace kagome::scale

#endif  // KAGOME_KAGOME_SCALE_HPP
