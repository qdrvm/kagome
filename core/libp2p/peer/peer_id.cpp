/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/peer/peer_id.hpp"

#include <boost/format.hpp>
#include "libp2p/multi/multihash.hpp"

namespace libp2p::peer {
  PeerId::PeerId(kagome::common::Buffer id) : id_{std::move(id)} {}

  PeerId::PeerId(kagome::common::Buffer id,
                 std::shared_ptr<crypto::PublicKey> public_key,
                 std::shared_ptr<crypto::PrivateKey> private_key)
      : id_{std::move(id)},
        public_key_{std::move(public_key)},
        private_key_{std::move(private_key)} {}

  const kagome::common::Buffer &PeerId::toBytes() const {
    return id_;
  }

  std::shared_ptr<crypto::PublicKey> PeerId::publicKey() const {
    return public_key_;
  }

  std::shared_ptr<crypto::PrivateKey> PeerId::privateKey() const {
    return private_key_;
  }

  bool PeerId::operator==(const PeerId &other) const {
    return this->id_ == other.id_ && this->private_key_ == other.private_key_
        && this->public_key_ == other.public_key_;
  }

  void PeerId::unsafeSetPublicKey(
      std::shared_ptr<crypto::PublicKey> public_key) {
    public_key_ = std::move(public_key);
  }

  void PeerId::unsafeSetPrivateKey(
      std::shared_ptr<crypto::PrivateKey> private_key) {
    private_key_ = std::move(private_key);
  }
}  // namespace libp2p::peer
