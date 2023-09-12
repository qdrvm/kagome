/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KAGOME_SCALE_HPP
#define KAGOME_KAGOME_SCALE_HPP

#include <type_traits>
#include "common/blob.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/babe/types/seal.hpp"
#include "network/types/roles.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_id.hpp"
#include "primitives/justification.hpp"
#include "scale/encode_append.hpp"
#include "scale/libp2p_types.hpp"

namespace kagome::scale {
  using CompactInteger = ::scale::CompactInteger;
  using BitVec = ::scale::BitVec;
  using ScaleDecoderStream = ::scale::ScaleDecoderStream;
  using ScaleEncoderStream = ::scale::ScaleEncoderStream;
  using PeerInfoSerializable = ::scale::PeerInfoSerializable;
  using DecodeError = ::scale::DecodeError;

  template <typename T>
  inline auto decode(gsl::span<const uint8_t> data) {
    return ::scale::decode<T>(std::move(data));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::BlockHeader &bh);

  template <typename F, typename ElementType, size_t MaxSize, typename... Args>
  constexpr void encode(
      const F &func, const common::SLVector<ElementType, MaxSize, Args...> &c);

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
  constexpr void encode(const F &func, const primitives::Seal &c);

  template <typename F>
  constexpr void encode(const F &func, const primitives::PreRuntime &c);

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
  constexpr void encode(const F &func,
                        const consensus::babe::BabeBlockHeader &bh) {
    encode(func, bh.slot_assignment_type);
    encode(func, bh.authority_index);
    encode(func, bh.slot_number);

    if (bh.needVRFCheck()) {
      encode(func, bh.vrf_output);
    }
  }

  template <typename F, typename ElementType, size_t MaxSize, typename... Args>
  constexpr void encode(
      const F &func, const common::SLVector<ElementType, MaxSize, Args...> &c) {
    encode(func, static_cast<const std::vector<ElementType, Args...> &>(c));
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
  constexpr void encode(const F &func, const primitives::Seal &c) {
    encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
  }

  template <typename F>
  constexpr void encode(const F &func, const primitives::PreRuntime &c) {
    encode(func, static_cast<const primitives::detail::DigestItemCommon &>(c));
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
