/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <libp2p/multi/multiaddress.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "scale/scale.hpp"

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

  enum class HttpError : int32_t {
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

  struct NoPayload {
    bool operator==(const NoPayload &) const {
      return true;
    }
  };
  SCALE_EMPTY_CODER(NoPayload);

  struct Success : public NoPayload {};
  struct Failure : public NoPayload {};

  template <typename S, typename F>
  struct Result final : boost::variant<S, F> {
    using Base = boost::variant<S, F>;
    using Base::Base;
    bool operator==(const Result &r_) const {
      const Base &l = *this, &r = r_;
      return l == r;
    }
    bool isSuccess() const {
      return boost::variant<S, F>::which() == 0;
    }
    bool isFailure() const {
      return boost::variant<S, F>::which() == 1;
    }
    S &value() {
      return boost::relaxed_get<S>(*this);
    }
    F &error() {
      return boost::relaxed_get<F>(*this);
    }
  };

  struct OpaqueNetworkState {
    libp2p::peer::PeerId peer_id;
    std::list<libp2p::multi::Multiaddress> address;

    OpaqueNetworkState(libp2p::peer::PeerId peer_id,
                       std::list<libp2p::multi::Multiaddress> address)
        : peer_id(std::move(peer_id)), address(std::move(address)) {}

    OpaqueNetworkState()
        : peer_id(libp2p::peer::PeerId::fromPublicKey(
                      libp2p::crypto::ProtobufKey{{}})
                      .value()) {}
  };

  template <class Stream>
    requires Stream::is_encoder_stream
  Stream &operator<<(Stream &s, const OpaqueNetworkState &v) {
    s << v.peer_id.toVector();

    s << scale::CompactInteger(v.address.size());

    for (auto &address : v.address) {
      s << address.getBytesAddress();
    }

    return s;
  }

  template <class Stream>
    requires Stream::is_decoder_stream
  Stream &operator>>(Stream &s, OpaqueNetworkState &v) {
    common::Buffer buff;

    s >> buff;
    auto peer_id_res = libp2p::peer::PeerId::fromBytes(buff);
    v.peer_id = std::move(peer_id_res.value());

    scale::CompactInteger size;
    s >> size;
    v.address.resize(size.convert_to<uint64_t>());

    for (auto &address : v.address) {
      s >> buff;
      auto ma_res = libp2p::multi::Multiaddress::create(buff);
      address = std::move(ma_res.value());
    }

    return s;
  }

}  // namespace kagome::offchain
