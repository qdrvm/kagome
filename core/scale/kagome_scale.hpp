/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KAGOME_SCALE_HPP
#define KAGOME_KAGOME_SCALE_HPP

#include <scale/encode_append.hpp>
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
#include "scale/libp2p_types.hpp"

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
  constexpr void encode(const F &func, const network::BlocksResponse &b);

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
    encode(func, bh.parent_hash);
    encodeCompact(func, bh.number);
    encode(func, bh.state_root);
    encode(func, bh.extrinsics_root);
    encode(func, bh.digest);
  }

  template <typename F>
  constexpr void encode(const F &func, const network::BlocksResponse &b) {
    encode(func, b.blocks);
  }

  template <typename F>
  constexpr void encode(const F &func,
                        const consensus::babe::BabeBlockHeader &bh) {
    encode(func, bh.slot_assignment_type);
    encode(func, bh.authority_index);
    encode(func, bh.slot_number);

    if (bh.needVRFCheck()) {
      encode(func, bh.vrf_output);
    }
  }

  template <typename F,
            typename ElementType,
            size_t MaxSize,
            typename Allocator>
  constexpr void encode(
      const F &func,
      const common::SLVector<ElementType, MaxSize, Allocator> &c) {
    encode(func, static_cast<const std::vector<ElementType, Allocator> &>(c));
  }

  template <typename F, typename T, typename Tag, typename Base>
  constexpr void encode(const F &func, const Tagged<T, Tag, Base> &c) {
    if constexpr (std::is_scalar_v<T>) {
      encode(func, c.Wrapper<T>::value);
    } else {
      encode(func, static_cast<const T &>(c));
    }
  }

  template <typename F, size_t MaxSize>
  constexpr void encode(const F &func, const common::SLBuffer<MaxSize> &c) {
    encode(func, static_cast<const common::SLVector<uint8_t, MaxSize> &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::Other &c) {
    encode(func, static_cast<const common::Buffer &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::Consensus &c) {
    encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func,
                        const kagome::runtime::PersistedValidationData &c) {
    encode(func, c.parent_head);
    encode(func, c.relay_parent_number);
    encode(func, c.relay_parent_storage_root);
    encode(func, c.max_pov_size);
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::Seal &c) {
    encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::PreRuntime &c) {
    encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockInfo &c) {
    encode(func, c.number);
    encode(func, c.hash);
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
    encode(func, c.value);
  }

}  // namespace kagome::scale

#endif  // KAGOME_KAGOME_SCALE_HPP
