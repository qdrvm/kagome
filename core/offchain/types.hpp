/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <common/empty.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <qtils/tagged.hpp>

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "scale/kagome_scale.hpp"

namespace kagome::offchain {

  /// Timestamp is milliseconds since UNIX Epoch
  using Timestamp = uint64_t;

  using RandomSeed = common::Blob<32>;

  // https://github.com/paritytech/polkadot-sdk/blob/7ecf3f757a5d6f622309cea7f788e8a547a5dce8/substrate/primitives/core/src/offchain/mod.rs#L63
  enum class StorageType : int32_t {
    Undefined = 0,  // shouldn't be used
    Persistent = 1,
    Local = 2
  };

  enum class HttpMethod { Undefined = 0, Get = 1, Post = 2 };

  using RequestId = int16_t;

  enum class HttpError : uint32_t {
    Timeout = 0,   //!< The deadline was reached
    IoError = 1,   //!< There was an IO error while processing the request
    InvalidId = 2  //!< The ID of the request is invalid
  };

  /**
   * @brief HTTP status codes that can get returned by certain Offchain funcs.
   * 0:  the specified request identifier is invalid.
   * 10: the deadline for the started request was reached.
   * 20: an error has occurred during the request, e.g. a timeout or the remote
   *     server has closed the connection. On returning this error code, the
   *     request is considered destroyed and must be reconstructed again.
   * 100-999: the request has finished with the given HTTP status code.
   */
  using HttpStatus = uint16_t;
  constexpr HttpStatus InvalidIdentifier(0);
  constexpr HttpStatus DeadlineHasReached(10);
  constexpr HttpStatus ErrorHasOccurred(20);

  struct Success : Empty {};
  struct Failure : Empty {};

  template <typename Succ, typename Fail>
  struct Result final : std::variant<Succ, Fail> {
    using Base = std::variant<Succ, Fail>;
    using Base::Base;
    bool operator==(const Result &r_) const {
      const Base &l = *this, &r = r_;
      return l == r;
    }
    bool isSuccess() const {
      return std::variant<Succ, Fail>::index() == 0;
    }
    bool isFailure() const {
      return std::variant<Succ, Fail>::index() == 1;
    }
    Succ &value() {
      return std::get<Succ>(*this);
    }
    Fail &error() {
      return std::get<Fail>(*this);
    }

    SCALE_CUSTOM_DECOMPOSITION(Result, SCALE_FROM_BASE(Base));
  };

  struct OpaqueNetworkState {
    libp2p::peer::PeerId peer_id;
    std::list<libp2p::multi::Multiaddress> address;

    OpaqueNetworkState(libp2p::peer::PeerId peer_id,
                       std::list<libp2p::multi::Multiaddress> address)
        : peer_id(std::move(peer_id)), address(std::move(address)) {}

    OpaqueNetworkState()
        : peer_id(
            libp2p::peer::PeerId::fromPublicKey(libp2p::crypto::ProtobufKey{{}})
                .value()) {}

    friend void encode(const OpaqueNetworkState &v, scale::Encoder &encoder) {
      encode(v.peer_id.toVector(), encoder);
      encode(scale::as_compact(v.address.size()), encoder);
      for (auto &address : v.address) {
        encode(address.getBytesAddress(), encoder);
      }
    }

    friend void decode(OpaqueNetworkState &v, scale::Decoder &decoder) {
      common::Buffer buff;
      decode(buff, decoder);
      auto peer_id_res = libp2p::peer::PeerId::fromBytes(buff);
      v.peer_id = std::move(peer_id_res.value());

      size_t size;
      decode(scale::as_compact(size), decoder);

      for (size_t i = 0; i < size; i++) {
        decode(buff, decoder);
        auto ma_res = libp2p::multi::Multiaddress::create(buff);
        v.address.emplace_back(std::move(ma_res.value()));
      }
    }
  };

}  // namespace kagome::offchain
