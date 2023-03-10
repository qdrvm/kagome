/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_PRIMITIVES_HPP
#define KAGOME_PARACHAIN_PRIMITIVES_HPP

#include <boost/variant.hpp>
#include <scale/bitvec.hpp>
#include <tuple>
#include <type_traits>
#include <vector>

#include "common/blob.hpp"
#include "consensus/grandpa/common.hpp"
#include "crypto/sr25519_types.hpp"
#include "primitives/block_header.hpp"
#include "primitives/common.hpp"
#include "primitives/compact_integer.hpp"
#include "primitives/digest.hpp"
#include "scale/tie.hpp"
#include "storage/trie/types.hpp"

namespace kagome::parachain {

  using ConstBuffer = kagome::common::Blob<32>;
  using Hash = common::Hash256;
  using Signature = kagome::crypto::Sr25519Signature;
  using ParachainId = uint32_t;
  using PublicKey = crypto::Sr25519PublicKey;
  using CollatorPublicKey = PublicKey;
  using ValidatorIndex = uint32_t;
  using ValidatorId = crypto::Sr25519PublicKey;
  using UpwardMessage = common::Buffer;
  using ParachainRuntime = common::Buffer;
  using HeadData = common::Buffer;
  using CandidateHash = Hash;
  using RelayHash = Hash;
  using ChunkProof = std::vector<common::Buffer>;
  using CandidateIndex = uint32_t;
  using CoreIndex = uint32_t;
  using GroupIndex = uint32_t;
  using CollatorId = CollatorPublicKey;
  using ValidationCodeHash = Hash;
  using BlockNumber = kagome::primitives::BlockNumber;
  using DownwardMessage = common::Buffer;
  using SessionIndex = uint32_t;
  using Tick = uint64_t;

  /// Validators assigning to check a particular candidate are split up into
  /// tranches. Earlier tranches of validators check first, with later tranches
  /// serving as backup.
  using DelayTranche = uint32_t;

  /// Signature with which parachain validators sign blocks.
  using ValidatorSignature = Signature;

  template <typename D>
  struct Indexed {
    using Type = std::decay_t<D>;
    SCALE_TIE(2)

    Type payload;
    ValidatorIndex ix;
  };

  template <typename T>
  using IndexedAndSigned = kagome::crypto::Sr25519Signed<Indexed<T>>;

  template <typename T>
  [[maybe_unused]] inline T const &getPayload(IndexedAndSigned<T> const &t) {
    return t.payload.payload;
  }

  template <typename T>
  [[maybe_unused]] inline T &getPayload(IndexedAndSigned<T> &t) {
    return t.payload.payload;
  }

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_PRIMITIVES_HPP
