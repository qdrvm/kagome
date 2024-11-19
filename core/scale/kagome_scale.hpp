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
#include "network/types/blocks_response.hpp"
#include "network/types/roles.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "primitives/justification.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "scale/big_fixed_integers.hpp"
#include "scale/encode_append.hpp"
#include "scale/libp2p_types.hpp"
//#include "network/types/collator_messages_vstaging.hpp"

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

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockHeader &bh);

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockHeaderReflection &bh);

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockReflection &bh);

  template <typename F>
  constexpr void encode(const F &func, const network::BlocksResponse &b);

//  template <typename F>
//  constexpr void encode(const F &func, const kagome::network::vstaging::AttestedCandidateRequest &b);

  template <typename F,
            typename ElementType,
            size_t MaxSize,
            typename Allocator>
  constexpr void encode(
      const F &func,
      const common::SLVector<ElementType, MaxSize, Allocator> &c);

  template <typename F, typename T, typename Tag, typename Base>
  constexpr void encode(const F &func, const Tagged<T, Tag, Base> &c);

  template <typename F, size_t MaxSize>
  constexpr void encode(const F &func, const common::SLBuffer<MaxSize> &c);

  template <typename F>
  constexpr void encode(const F &func, const network::Roles &c);

  template <typename F>
  constexpr void encode(const F &func, const primitives::Other &c);

  template <typename F>
  constexpr void encode(const F &func, const primitives::Consensus &c);

  template <typename F>
  constexpr void encode(const F &func,
                        const runtime::PersistedValidationData &c);

  template <typename F>
  constexpr void encode(const F &func, const primitives::Seal &c);

  template <typename F>
  constexpr void encode(const F &func, const primitives::PreRuntime &c);

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockInfo &c);

  template <typename F>
  constexpr void encode(const F &func,
                        const primitives::RuntimeEnvironmentUpdated &c);

  template <typename F>
  constexpr void encode(const F &func, const ::scale::EncodeOpaqueValue &c);

  template <typename F>
  constexpr void encode(const F &func,
                        const consensus::babe::BabeBlockHeader &bh);

}  // namespace kagome::scale

#include "scale/encoder/primitives.hpp"

namespace kagome::scale {

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockHeader &bh) {
    kagome::scale::encode(func, bh.parent_hash);
    kagome::scale::encodeCompact(func, bh.number);
    kagome::scale::encode(func, bh.state_root);
    kagome::scale::encode(func, bh.extrinsics_root);
    kagome::scale::encode(func, bh.digest);
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockReflection &bh) {
    kagome::scale::encode(func, bh.header);
    kagome::scale::encode(func, bh.body);
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockHeaderReflection &bh) {
    kagome::scale::encode(func, bh.parent_hash);
    kagome::scale::encodeCompact(func, bh.number);
    kagome::scale::encode(func, bh.state_root);
    kagome::scale::encode(func, bh.extrinsics_root);
    kagome::scale::encode(func, bh.digest);
  }

  template <typename F>
  constexpr void encode(const F &func, const network::BlocksResponse &b) {
    kagome::scale::encode(func, b.blocks);
  }

//  template <typename F>
//  constexpr void encode(const F &func, const kagome::network::vstaging::AttestedCandidateRequest &b) {
//    encode(func, b.candidate_hash);
//    encode(func, b.mask);
//  }

  template <typename F>
  constexpr void encode(const F &func,
                        const consensus::babe::BabeBlockHeader &bh) {
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
      const common::SLVector<ElementType, MaxSize, Allocator> &c) {
    kagome::scale::encode(func, static_cast<const std::vector<ElementType, Allocator> &>(c));
  }

  template <typename F, typename T, typename Tag, typename Base>
  constexpr void encode(const F &func, const Tagged<T, Tag, Base> &c) {
    if constexpr (std::is_scalar_v<T>) {
      kagome::scale::encode(func, c.Wrapper<T>::value);
    } else {
      kagome::scale::encode(func, static_cast<const T &>(c));
    }
  }

  template <typename F, size_t MaxSize>
  constexpr void encode(const F &func, const common::SLBuffer<MaxSize> &c) {
    kagome::scale::encode(func, static_cast<const common::SLVector<uint8_t, MaxSize> &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::Other &c) {
    kagome::scale::encode(func, static_cast<const common::Buffer &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::Consensus &c) {
    kagome::scale::encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func,
                        const kagome::runtime::PersistedValidationData &c) {
    kagome::scale::encode(func, c.parent_head);
    kagome::scale::encode(func, c.relay_parent_number);
    kagome::scale::encode(func, c.relay_parent_storage_root);
    kagome::scale::encode(func, c.max_pov_size);
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::Seal &c) {
    kagome::scale::encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::PreRuntime &c) {
    kagome::scale::encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockInfo &c) {
    kagome::scale::encode(func, c.number);
    kagome::scale::encode(func, c.hash);
  }

  template <typename F>
  constexpr void encode(const F &func,
                        const primitives::RuntimeEnvironmentUpdated &c) {}

  template <typename F>
  constexpr void encode(const F &func, const ::scale::EncodeOpaqueValue &c) {
    putByte(func, c.v.data(), c.v.size());
  }

  template <typename F>
  constexpr void encode(const F &func, const network::Roles &c) {
    kagome::scale::encode(func, c.value);
  }

}  // namespace kagome::scale

#endif  // KAGOME_KAGOME_SCALE_HPP
