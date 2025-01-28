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
#include "consensus/babe/types/seal.hpp"
#include "consensus/beefy/types.hpp"
#include "consensus/grandpa/types/equivocation_proof.hpp"
#include "network/types/blocks_response.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "network/types/dispute_messages.hpp"
#include "network/types/roles.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "primitives/justification.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "scale/big_fixed_integers.hpp"
#include "scale/encode_append.hpp"
#include "scale/encoder/concepts.hpp"
#include "scale/libp2p_types.hpp"

#include <authority_discovery/query/authority_peer_info.hpp>

namespace kagome::scale {
  using CompactInteger = ::scale::CompactInteger;
  using BitVec = ::scale::BitVec;
  using ScaleDecoderStream = ::scale::ScaleDecoderStream;
  using ScaleEncoderStream = ::scale::ScaleEncoderStream;
  using PeerInfoSerializable = ::scale::PeerInfoSerializable;
  using DecodeError = ::scale::DecodeError;
  template <typename T>
  using Fixed = ::scale::Fixed<T>;
  template <typename T>
  using Compact = ::scale::Compact<T>;
  using uint128_t = ::scale::uint128_t;

  using ::scale::decode;

  constexpr void encode(const Invocable auto &func,
                        const primitives::BlockHeader &bh);

  constexpr void encode(const Invocable auto &func,
                        const consensus::grandpa::Equivocation &bh);

  constexpr void encode(const Invocable auto &func,
                        const primitives::BlockHeaderReflection &bh);

  constexpr void encode(const Invocable auto &func,
                        const primitives::BlockReflection &bh);

  constexpr void encode(const Invocable auto &func,
                        const network::BlocksResponse &b);

  constexpr void encode(const Invocable auto &func,
                        const kagome::network::vstaging::CompactStatement &c);

  template <typename ElementType, size_t MaxSize, typename Allocator>
  constexpr void encode(
      const Invocable auto &func,
      const common::SLVector<ElementType, MaxSize, Allocator> &c);

  template <typename T, typename Tag>
  constexpr void encode(const Invocable auto &func, const Tagged<T, Tag> &c);

  template <size_t MaxSize>
  constexpr void encode(const Invocable auto &func,
                        const common::SLBuffer<MaxSize> &c);

  constexpr void encode(const Invocable auto &func, const network::Roles &c);

  constexpr void encode(const Invocable auto &func, const primitives::Other &c);

  constexpr void encode(const Invocable auto &func,
                        const primitives::Consensus &c);

  constexpr void encode(const Invocable auto &func,
                        const runtime::PersistedValidationData &c);

  constexpr void encode(const Invocable auto &func, const primitives::Seal &c);

  constexpr void encode(const Invocable auto &func,
                        const primitives::PreRuntime &c);

  constexpr void encode(const Invocable auto &func,
                        const primitives::BlockInfo &c);

  constexpr void encode(const Invocable auto &func,
                        const primitives::RuntimeEnvironmentUpdated &c);

  constexpr void encode(const Invocable auto &func,
                        const ::scale::EncodeOpaqueValue &c);

  constexpr void encode(const Invocable auto &func,
                        const consensus::babe::BabeBlockHeader &bh);

  constexpr void encode(const Invocable auto &func,
                        const kagome::network::CandidateCommitments &c);

  constexpr void encode(const Invocable auto &func,
                        const kagome::network::CandidateReceipt &c);

  constexpr void encode(const Invocable auto &func,
                        const kagome::network::InvalidDisputeVote &c);

  constexpr void encode(const Invocable auto &func,
                        const kagome::network::ValidDisputeVote &c);

  constexpr void encode(const Invocable auto &func,
                        const consensus::grandpa::SignedPrecommit &c);
  template <typename F>
  constexpr void encode(const F &func,
                        const authority_discovery::AuthorityPeerInfo &c);

  constexpr void encode(const Invocable auto &func,
                        const scale::PeerInfoSerializable &c);

  template <typename T>
  constexpr void encode(const Invocable auto &func, const Fixed<T> &c);
}  // namespace kagome::scale

#include "scale/encoder/primitives.hpp"

namespace kagome::scale {

  constexpr void encode(const Invocable auto &func,
                        const primitives::BlockHeader &bh) {
    kagome::scale::encode(func, bh.parent_hash);
    kagome::scale::encodeCompact(func, bh.number);
    kagome::scale::encode(func, bh.state_root);
    kagome::scale::encode(func, bh.extrinsics_root);
    kagome::scale::encode(func, bh.digest);
  }

  constexpr void encode(const Invocable auto &func,
                        const primitives::BlockReflection &bh) {
    kagome::scale::encode(func, bh.header);
    kagome::scale::encode(func, bh.body);
  }

  constexpr void encode(const Invocable auto &func,
                        const primitives::BlockHeaderReflection &bh) {
    kagome::scale::encode(func, bh.parent_hash);
    kagome::scale::encodeCompact(func, bh.number);
    kagome::scale::encode(func, bh.state_root);
    kagome::scale::encode(func, bh.extrinsics_root);
    kagome::scale::encode(func, bh.digest);
  }

  constexpr void encode(const Invocable auto &func,
                        const network::BlocksResponse &b) {
    kagome::scale::encode(func, b.blocks);
  }

  constexpr void encode(const Invocable auto &func,
                        const consensus::babe::BabeBlockHeader &bh) {
    kagome::scale::encode(func, bh.slot_assignment_type);
    kagome::scale::encode(func, bh.authority_index);
    kagome::scale::encode(func, bh.slot_number);

    if (bh.needVRFCheck()) {
      kagome::scale::encode(func, bh.vrf_output);
    }
  }

  template <typename ElementType, size_t MaxSize, typename Allocator>
  constexpr void encode(
      const Invocable auto &func,
      const common::SLVector<ElementType, MaxSize, Allocator> &c) {
    kagome::scale::encode(
        func, static_cast<const std::vector<ElementType, Allocator> &>(c));
  }

  template <typename T, typename Tag>
  constexpr void encode(const Invocable auto &func, const Tagged<T, Tag> &c) {
    if constexpr (std::is_scalar_v<T>) {
      kagome::scale::encode(func, c.template Wrapper<T>::value);
    } else {
      kagome::scale::encode(func, static_cast<const T &>(c));
    }
  }

  template <size_t MaxSize>
  constexpr void encode(const Invocable auto &func,
                        const common::SLBuffer<MaxSize> &c) {
    kagome::scale::encode(
        func, static_cast<const common::SLVector<uint8_t, MaxSize> &>(c));
  }

  constexpr void encode(const Invocable auto &func,
                        const primitives::Other &c) {
    kagome::scale::encode(func, static_cast<const common::Buffer &>(c));
  }

  constexpr void encode(const Invocable auto &func,
                        const primitives::Consensus &c) {
    kagome::scale::encode(
        func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  constexpr void encode(const Invocable auto &func,
                        const kagome::runtime::PersistedValidationData &c) {
    kagome::scale::encode(func, c.parent_head);
    kagome::scale::encode(func, c.relay_parent_number);
    kagome::scale::encode(func, c.relay_parent_storage_root);
    kagome::scale::encode(func, c.max_pov_size);
  }

  constexpr void encode(const Invocable auto &func, const primitives::Seal &c) {
    kagome::scale::encode(
        func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  constexpr void encode(const Invocable auto &func,
                        const primitives::PreRuntime &c) {
    kagome::scale::encode(
        func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  constexpr void encode(const Invocable auto &func,
                        const primitives::BlockInfo &c) {
    kagome::scale::encode(func, c.number);
    kagome::scale::encode(func, c.hash);
  }

  constexpr void encode(const Invocable auto &func,
                        const primitives::RuntimeEnvironmentUpdated &c) {}

  constexpr void encode(const Invocable auto &func,
                        const ::scale::EncodeOpaqueValue &c) {
    putByte(func, c.v.data(), c.v.size());
  }

  constexpr void encode(const Invocable auto &func, const network::Roles &c) {
    kagome::scale::encode(
        func, c.value);  // NOLINT(cppcoreguidelines-pro-type-union-access)
  }

  constexpr void encode(const Invocable auto &func,
                        const consensus::grandpa::Equivocation &bh) {
    kagome::scale::encode(func, bh.stage);
    kagome::scale::encode(func, bh.round_number);
    kagome::scale::encode(func, bh.first);
    kagome::scale::encode(func, bh.second);
  }

  constexpr void encode(const Invocable auto &func,
                        const kagome::network::CandidateCommitments &c) {
    kagome::scale::encode(func, c.upward_msgs);
    kagome::scale::encode(func, c.outbound_hor_msgs);
    kagome::scale::encode(func, c.opt_para_runtime);
    kagome::scale::encode(func, c.para_head);
    kagome::scale::encode(func, c.downward_msgs_count);
    kagome::scale::encode(func, c.watermark);
  }

  constexpr void encode(const Invocable auto &func,
                        const kagome::network::CandidateReceipt &c) {
    kagome::scale::encode(func, c.descriptor);
    kagome::scale::encode(func, c.commitments_hash);
  }

  constexpr void encode(const Invocable auto &func,
                        const kagome::network::vstaging::CompactStatement &c) {
    kagome::scale::encode(func, c.header);
    kagome::scale::encode(func, c.inner_value);
  }

  constexpr void encode(const Invocable auto &func,
                        const kagome::network::InvalidDisputeVote &c) {
    kagome::scale::encode(func, c.index);
    kagome::scale::encode(func, c.signature);
    kagome::scale::encode(func, c.kind);
  }

  constexpr void encode(const Invocable auto &func,
                        const kagome::network::ValidDisputeVote &c) {
    kagome::scale::encode(func, c.index);
    kagome::scale::encode(func, c.signature);
    kagome::scale::encode(func, c.kind);
  }

  constexpr void encode(const Invocable auto &func,
                        const consensus::grandpa::SignedPrecommit &c) {
    kagome::scale::encode(
        func, static_cast<const consensus::grandpa::SignedMessage &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func,
                        const authority_discovery::AuthorityPeerInfo &c) {
    encode(func, c.raw);
    encode(func, c.time);
    encode(func, c.peer);
  }

  constexpr void encode(const Invocable auto &func,
                        const scale::PeerInfoSerializable &c) {
    std::vector<std::string> addresses;
    addresses.reserve(c.addresses.size());
    for (const auto &addr : c.addresses) {
      addresses.emplace_back(addr.getStringAddress());
    }
    encode(func, c.id.toBase58());
    encode(func, addresses);
  }

  template <typename T>
  constexpr void encode(const Invocable auto &func, const Fixed<T> &fixed) {
    T original = untagged(fixed);
    for (size_t i = 0; i < ::scale::IntegerTraits<T>::kBitSize; i += 8) {
      encode(func, ::scale::convert_to<uint8_t>((original >> i) & 0xFFu));
    }
  }

}  // namespace kagome::scale

#endif  // KAGOME_KAGOME_SCALE_HPP
