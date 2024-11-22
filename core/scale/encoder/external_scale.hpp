/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "scale/big_fixed_integers.hpp"
#include "scale/encode_append.hpp"
#include "scale/encoder/concepts.hpp"
#include "scale/libp2p_types.hpp"
#include <scale/types.hpp>
#include "common/size_limited_containers.hpp"
//#include "common/tagged.hpp"
//#include "common/buffer.hpp"
//#include <scale/scale.hpp>

namespace kagome {
    namespace primitives {
        struct BlockHeader;
        struct BlockHeaderReflection;
        struct BlockReflection;
        struct Other;
        struct Consensus;
        struct Seal;
        struct PreRuntime;
        struct RuntimeEnvironmentUpdated;
        namespace detail {
            template <typename Tag> struct BlockInfoT;
        }
        using BlockInfo = detail::BlockInfoT<struct BlockInfoTag>;
    }
    namespace consensus {
        namespace grandpa {
            struct Equivocation;
            class SignedPrecommit;
        }
        namespace babe {
            struct BabeBlockHeader;
        }
    }
    namespace network {
        struct BlocksResponse;
        struct Roles;
        struct CandidateCommitments;
        struct CandidateReceipt;
        struct InvalidDisputeVote;
        struct ValidDisputeVote;
        namespace vstaging {
            struct CompactStatement;
        }
    }
    namespace runtime {
        struct PersistedValidationData;
    }
    namespace crypto {
        struct EcdsaSignature;
        struct EcdsaPublicKey;
    }
    template <typename T, typename >
  struct Wrapper;
    template <typename T,
            typename Tag,
            typename Base >
    class Tagged;
}

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
  using Timestamp = scale::uint128_t;
  using TimestampScale = scale::Fixed<Timestamp>;

  using ::scale::decode;
  using ::scale::append_or_new_vec;

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

  template <typename T, typename Tag, typename Base>
  constexpr void encode(const Invocable auto &func,
                        const Tagged<T, Tag, Base> &c);

  template <size_t MaxSize, typename Allocator>
  constexpr void encode(const Invocable auto &func,
                        const common::SLBuffer<MaxSize, Allocator> &c);

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

  constexpr void putByte(const Invocable auto &func,
                         const uint8_t *const val,
                         size_t count);

  template <typename... Ts>
  constexpr void encode(const Invocable auto &func, const std::tuple<Ts...> &v);

  template <typename T, typename... Args>
  constexpr void encode(const Invocable auto &func,
                        const T &t,
                        const Args &...args);

  template <typename T>
  constexpr void encode(const Invocable auto &func, const std::vector<T> &c);

  template <typename F, typename S>
  constexpr void encode(const Invocable auto &func, const std::pair<F, S> &p);

  template <typename T, ssize_t S>
  constexpr void encode(const Invocable auto &func, const std::span<T, S> &c);

  template <typename T>
  constexpr void encode(const Invocable auto &func, const std::span<T> &c);

  template <typename T, size_t size>
  constexpr void encode(const Invocable auto &func,
                        const std::array<T, size> &c);

  template <typename T, size_t N>
  constexpr void encode(const Invocable auto &func, const T (&c)[N]);

  template <typename K, typename V>
  constexpr void encode(const Invocable auto &func, const std::map<K, V> &c);

  template <typename T>
  constexpr void encode(const Invocable auto &func,
                        const std::shared_ptr<T> &v);

  constexpr void encode(const Invocable auto &func, const std::string_view &v);

  constexpr void encode(const Invocable auto &func, const std::string &v);
  
  template <typename T, size_t S>
  constexpr void encode(const Invocable auto &func, const std::span<T, S> &c);

  template <typename T>
  constexpr void encode(const Invocable auto &func,
                        const std::unique_ptr<T> &v);

  template <typename T>
  constexpr void encode(const Invocable auto &func, const std::list<T> &c);

  template <typename T>
  constexpr void encode(const Invocable auto &func, const std::deque<T> &c);

  template <uint8_t I, typename... Ts>
  void encode(const Invocable auto &func, const boost::variant<Ts...> &v);

  template <typename... Ts>
  void encode(const Invocable auto &func, const boost::variant<Ts...> &v);

  template <typename... Ts>
  void encode(const Invocable auto &func, const std::variant<Ts...> &v);

  void encode(const Invocable auto &func, const ::scale::CompactInteger &value);

  void encode(const Invocable auto &func, const ::scale::BitVec &value);

  void encode(const Invocable auto &func, const std::optional<bool> &value);

  template <typename T>
  void encode(const Invocable auto &func, const std::optional<T> &value);

  void encode(const Invocable auto &func, const crypto::EcdsaSignature &value);

  void encode(const Invocable auto &func, const crypto::EcdsaPublicKey &value);

  constexpr void encode(const Invocable auto &func, const IsNotEnum auto &v);

  constexpr void encode(const Invocable auto &func, const IsEnum auto &value);

  template <typename T, typename... Args>
  constexpr void encode(const Invocable auto &func,
                        const T &t,
                        const Args &...args);
 
 template <typename... Args>
  outcome::result<std::vector<uint8_t>> encode(const Args &...args);
}  // namespace kagome::scale
