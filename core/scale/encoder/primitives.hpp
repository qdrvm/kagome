/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
#include "scale/encoder/external_scale.hpp"
#include <any>
#include <deque>
#include <iostream>
#include <map>
#include <optional>
#include <scale/outcome/outcome_throw.hpp>
#include <scale/types.hpp>
#include <span>
#include <tuple>
#include <type_traits>
#include <vector>
#include "crypto/ecdsa_types.hpp"
#include "scale/encoder/concepts.hpp"

#include "utils/struct_to_tuple.hpp"

namespace kagome::scale {

  constexpr void encode(const Invocable auto &func, const IsNotEnum auto &v) {
    using I = std::decay_t<decltype(v)>;
    if constexpr (std::is_integral_v<I>) {
      if constexpr (std::is_same_v<I, bool>) {
        const uint8_t byte = (v ? 1u : 0u);
        putByte(func, &byte, 1ul);
        return;
      }

      if constexpr (sizeof(I) == 1u) {
        putByte(func, (const uint8_t *)&v, size_t(1ull));
        return;
      }

      constexpr size_t size = sizeof(I);
      const auto val = math::toLE(v);
      putByte(func, (uint8_t *)&val, size);
    } else {
      kagome::scale::encode(func, utils::to_tuple_refs(v));
    }
  }

  constexpr void encode(const Invocable auto &func, const IsEnum auto &value) {
    kagome::scale::encode(
        func,
        static_cast<std::underlying_type_t<std::decay_t<decltype(value)>>>(
            value));
  }

  inline size_t bitUpperBorder(const ::scale::CompactInteger &x) {
    namespace mp = boost::multiprecision;
    const size_t size = x.backend().size();
    const mp::limb_type *const p = x.backend().limbs();

    auto counter = (size - 1) * sizeof(mp::limb_type) * 8;
    auto value = p[size - 1];

    if constexpr (sizeof(mp::limb_type) == sizeof(uint64_t)) {
      counter += sizeof(uint64_t) * 8
               - (value == 0 ? sizeof(uint64_t) * 8 : __builtin_clzll(value));
    } else if constexpr (sizeof(mp::limb_type) == sizeof(uint32_t)) {
      counter += sizeof(uint32_t) * 8
               - (value == 0 ? sizeof(uint32_t) * 8 : __builtin_clz(value));
    }
    return counter;
  }

  inline size_t countBytes(::scale::CompactInteger x) {
    if (x == 0) {
      return 1ull;
    }
    namespace mp = boost::multiprecision;
    const size_t size = x.backend().size();
    const mp::limb_type *const p = x.backend().limbs();

    auto counter = (size - 1) * sizeof(mp::limb_type);
    auto value = p[size - 1];

    static_assert(sizeof(mp::limb_type) >= sizeof(uint32_t),
                  "Unexpected limb size");
    if constexpr (sizeof(mp::limb_type) == sizeof(uint64_t)) {
      counter += 1 + ((sizeof(uint64_t) * 8 - __builtin_clzll(value)) - 1) / 8;
    } else if constexpr (sizeof(mp::limb_type) == sizeof(uint32_t)) {
      counter += 1 + ((sizeof(uint32_t) * 8 - __builtin_clz(value)) - 1) / 8;
    }
    return counter;
  }

  constexpr void putByte(const Invocable auto &func,
                         const uint8_t *const val,
                         size_t count) {
    func(val, count);
  }

  template <uint8_t I, typename... Ts>
  void encode(const Invocable auto &func, const boost::variant<Ts...> &v) {
    using T = std::tuple_element_t<I, std::tuple<Ts...>>;
    if (v.which() == I) {
      kagome::scale::encode(func, I);
      kagome::scale::encode(func, boost::get<T>(v));
      return;
    }
    if constexpr (sizeof...(Ts) > I + 1) {
      kagome::scale::encode<I + 1>(func, v);
    }
  }

  template <typename T, size_t N>
  constexpr void encode(const Invocable auto &func, const T (&c)[N]) {
    using E = std::decay_t<T>;
    if constexpr (std::is_integral_v<E> && sizeof(E) == 1u) {
      putByte(func, c, N);
    } else {
      for (const auto &e : c) {
        kagome::scale::encode(func, e);
      }
    }
  }

  template <typename... Ts>
  void encode(const Invocable auto &func, const boost::variant<Ts...> &v) {
    kagome::scale::encode<0>(func, v);
  }

  template <typename T,
            typename I = std::decay_t<T>,
            std::enable_if_t<std::is_unsigned_v<I>, bool> = true>
  constexpr void encodeCompactSmall(const Invocable auto &func, T val) {
    BOOST_ASSERT_MSG((val >> (8 * sizeof(I) - 2)) == 0,
                     "Unexpected compact value in encoder");
    val <<= 2;
    val |= (sizeof(I) / 2ull);

    kagome::scale::encode(func, val);
  }

  void encodeCompact(const Invocable auto &func, uint64_t val) {
    if (val < ::scale::compact::EncodingCategoryLimits::kMinUint16) {
      kagome::scale::encodeCompactSmall(func, static_cast<uint8_t>(val));
      return;
    }

    if (val < ::scale::compact::EncodingCategoryLimits::kMinUint32) {
      kagome::scale::encodeCompactSmall(func, static_cast<uint16_t>(val));
      return;
    }

    if (val < ::scale::compact::EncodingCategoryLimits::kMinBigInteger) {
      kagome::scale::encodeCompactSmall(func, static_cast<uint32_t>(val));
      return;
    }

    const size_t bigIntLength = sizeof(uint64_t) - (__builtin_clzll(val) / 8);

    uint8_t result[sizeof(uint64_t) + sizeof(uint8_t)];
    result[0] = (bigIntLength - 4) * 4 + 3;  // header

    *(uint64_t *)&result[1] = math::toLE(val);
    putByte(func, result, bigIntLength + 1ull);
  }

  template <typename... Ts>
  void encode(const Invocable auto &func, const std::variant<Ts...> &v) {
    kagome::scale::encode(func, (uint8_t)v.index());
    std::visit([&](const auto &s) { kagome::scale::encode(func, s); }, v);
  }

  constexpr void encode(const Invocable auto &func, const std::string &v) {
    kagome::scale::encode(func, std::string_view{v});
  }

  constexpr void encode(const Invocable auto &func, const std::string_view &v) {
    kagome::scale::encodeCompact(func, v.size());
    putByte(func, (const uint8_t *)v.data(), v.size());
  }

  void encode(const Invocable auto &func, const ::scale::BitVec &v) {
    const size_t bitsCount = v.bits.size();
    const size_t bytesCount = ((bitsCount + 7ull) >> 3ull);
    const size_t blocksCount = ((bytesCount + 7ull) >> 3ull);

    kagome::scale::encodeCompact(func, bitsCount);
    uint64_t result;
    size_t bitCounter = 0ull;
    for (size_t ix = 0ull; ix < blocksCount; ++ix) {
      result = 0ull;
      size_t remains = std::min(size_t(64ull), bitsCount - bitCounter);
      do {
        result |= ((v.bits[bitCounter] ? 1ull : 0ull) << (bitCounter % 64ull));
        ++bitCounter;
      } while (--remains);

      const size_t bits = (bitCounter % 64ull);
      const size_t bytes =
          (bits != 0ull) ? ((bits + 7ull) >> 3ull) : sizeof(result);

      result = math::toLE(result);
      putByte(func, (uint8_t *)&result, bytes);
    }
  }

  void encode(const Invocable auto &func,
              const ::scale::CompactInteger &value) {
    if (value < 0) {
      raise(::scale::EncodeError::NEGATIVE_COMPACT_INTEGER);
    }

    const size_t bit_border = bitUpperBorder(value);
    if (bit_border <= 6) {  // kMinUint16
      kagome::scale::encodeCompactSmall(func, value.convert_to<uint8_t>());
      return;
    }

    if (bit_border <= 14) {  // kMinUint32
      kagome::scale::encodeCompactSmall(func, value.convert_to<uint16_t>());
      return;
    }

    if (bit_border <= 30) {  // kMinBigInteger
      kagome::scale::encodeCompactSmall(func, value.convert_to<uint32_t>());
      return;
    }

    constexpr size_t kReserved = 68ull;
    const size_t bigIntLength = countBytes(value);

    if (bigIntLength >= kReserved) {
      raise(::scale::EncodeError::COMPACT_INTEGER_TOO_BIG);
    }

    namespace mp = boost::multiprecision;
    constexpr size_t limb_sz = sizeof(mp::limb_type);

    uint8_t result[1 + ((kReserved + limb_sz - 1) / limb_sz) * limb_sz];
    result[0] = (bigIntLength - 4) * 4 + 3;  // header

    const size_t size = value.backend().size();
    const mp::limb_type *const p = value.backend().limbs();

    size_t ix = 0ull;
    for (; ix < size; ++ix) {
      *(mp::limb_type *)&result[1ull + ix * limb_sz] = math::toLE(p[ix]);
    }

    putByte(func, result, bigIntLength + 1ull);
  }

  template <
      typename It,
      typename = std::enable_if_t<
          !std::is_same_v<typename std::iterator_traits<It>::value_type, void>>>
  constexpr void encode(const Invocable auto &func, It begin, It end) {
    while (begin != end) {
      kagome::scale::encode(func, *begin);
      ++begin;
    }
  }

  template <typename T>
  constexpr void encode(const Invocable auto &func, const std::span<T> &c) {
    kagome::scale::encodeCompact(func, c.size());
    kagome::scale::encode(func, c.begin(), c.end());
  }

  template <typename T, size_t S>
  constexpr void encode(const Invocable auto &func, const std::span<T, S> &c) {
      using E = std::decay_t<T>;
      if constexpr (std::is_integral_v<E> && sizeof(E) == 1u) {
        putByte(func, c.data(), c.size());
      } else {
        for (const auto &e : c) {
          encode(func, e);
        }
      }
  }

  template <typename T, ssize_t S>
  constexpr void encode(const Invocable auto &func, const std::span<T, S> &c) {
    if constexpr (S == -1) {
      kagome::scale::encodeCompact(func, c.size());
      kagome::scale::encode(func, c.begin(), c.end());
    } else {
      kagome::scale::encode<T, size_t(S)>(func, c);

      //using E = std::decay_t<T>;
      //if constexpr (std::is_integral_v<E> && sizeof(E) == 1u) {
      //  putByte(func, c.data(), c.size());
      //} else {
      //  for (const auto &e : c) {
      //    encode(func, e);
      //  }
      //}
    }
  }

  template <typename T, size_t size>
  constexpr void encode(const Invocable auto &func,
                        const std::array<T, size> &c) {
    for (const auto &e : c) {
      kagome::scale::encode(func, e);
    }
  }

  template <typename K, typename V>
  constexpr void encode(const Invocable auto &func, const std::map<K, V> &c) {
    kagome::scale::encodeCompact(func, c.size());
    kagome::scale::encode(func, c.begin(), c.end());
  }

  template <typename F, typename S>
  constexpr void encode(const Invocable auto &func, const std::pair<F, S> &p) {
    kagome::scale::encode(func, p.first);
    kagome::scale::encode(func, p.second);
  }

  template <typename T>
  constexpr void encode(const Invocable auto &func, const std::vector<T> &c) {
    kagome::scale::encodeCompact(func, c.size());
    kagome::scale::encode(func, c.begin(), c.end());
  }

  template <typename T>
  constexpr void encode(const Invocable auto &func,
                        const std::shared_ptr<T> &v) {
    if (v == nullptr) {
      raise(::scale::EncodeError::DEREF_NULLPOINTER);
    }
    kagome::scale::encode(func, *v);
  }

  template <typename T>
  constexpr void encode(const Invocable auto &func,
                        const std::unique_ptr<T> &v) {
    if (v == nullptr) {
      raise(::scale::EncodeError::DEREF_NULLPOINTER);
    }
    kagome::scale::encode(func, *v);
  }

  template <typename T>
  constexpr void encode(const Invocable auto &func, const std::list<T> &c) {
    kagome::scale::encodeCompact(func, c.size());
    kagome::scale::encode(func, c.begin(), c.end());
  }

  template <typename T>
  constexpr void encode(const Invocable auto &func, const std::deque<T> &c) {
    kagome::scale::encodeCompact(func, c.size());
    kagome::scale::encode(func, c.begin(), c.end());
  }

  template <typename... Ts>
  constexpr void encode(const Invocable auto &func,
                        const std::tuple<Ts...> &v) {
    if constexpr (sizeof...(Ts) > 0) {
      std::apply(
          [&](const auto &...s) { (..., kagome::scale::encode(func, s)); }, v);
    }
  }

  void encode(const Invocable auto &func, const std::optional<bool> &v) {
    enum class OptionalBool : uint8_t {
      NONE = 0u,
      OPT_TRUE = 1u,
      OPT_FALSE = 2u
    };

    auto result = OptionalBool::OPT_TRUE;
    if (!v.has_value()) {
      result = OptionalBool::NONE;
    } else if (!*v) {
      result = OptionalBool::OPT_FALSE;
    }
    kagome::scale::encode(func, result);
  }

  template <typename T>
  void encode(const Invocable auto &func, const std::optional<T> &v) {
    if (!v.has_value()) {
      kagome::scale::encode(func, uint8_t(0u));
    } else {
      kagome::scale::encode(func, uint8_t(1u));
      kagome::scale::encode(func, *v);
    }
  }

  void encode(const Invocable auto &func, const crypto::EcdsaSignature &data) {
    kagome::scale::encode(
        func,
        static_cast<const common::Blob<crypto::constants::ecdsa::SIGNATURE_SIZE>
                        &>(data));
  }

  void encode(const Invocable auto &func, const crypto::EcdsaPublicKey &data) {
    kagome::scale::encode(
        func,
        static_cast<
            const common::Blob<crypto::constants::ecdsa::PUBKEY_SIZE> &>(data));
  }


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

  template <typename T, typename Tag, typename Base>
  constexpr void encode(const Invocable auto &func,
                        const Tagged<T, Tag, Base> &c) {
    if constexpr (std::is_scalar_v<T>) {
      kagome::scale::encode(func, c.Wrapper<T>::value);
    } else {
      kagome::scale::encode(func, static_cast<const T &>(c));
    }
  }

  template <size_t MaxSize, typename Allocator>
  constexpr void encode(const Invocable auto &func,
                        const common::SLBuffer<MaxSize, Allocator> &c) {
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
    kagome::scale::encode(func, c.value);
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

  template <typename T, typename... Args>
  constexpr void encode(const Invocable auto &func,
                        const T &t,
                        const Args &...args) {
    kagome::scale::encode(func, t);
    kagome::scale::encode(func, args...);
  }

  template <typename... Args>
  outcome::result<std::vector<uint8_t>> encode(const Args &...args) {
    std::vector<uint8_t> res;
    kagome::scale::encode(
        [&](const uint8_t *const val, size_t count) {
          if (count != 0ull) {
            res.insert(res.end(), &val[0], &val[count]);
          }
        },
        args...);
    return res;
  }
}  // namespace kagome::scale
