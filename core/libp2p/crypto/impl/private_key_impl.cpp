/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "private_key_impl.hpp"
#include "libp2p/crypto/impl/public_key_impl.hpp"

namespace libp2p::crypto {

  crypto::PrivateKeyImpl::PrivateKeyImpl(common::KeyType key_type,
                                         kagome::common::Buffer &&bytes)
      : key_type_{key_type}, bytes_(bytes) {}

  PrivateKeyImpl::PrivateKeyImpl(common::KeyType key_type,
                                 PrivateKeyImpl::Buffer bytes)
      : key_type_{key_type}, bytes_(std::move(bytes)) {}

  common::KeyType PrivateKeyImpl::getType() const {
    return key_type_;
  }

  const PrivateKeyImpl::Buffer &PrivateKeyImpl::getBytes() const {
    return bytes_;
  }

  bool PrivateKeyImpl::operator==(const Key &other) const {
    return key_type_ == other.getType() && bytes_ == other.getBytes();
  }

  std::shared_ptr<PublicKey>PrivateKeyImpl::publicKey() const {
    // TODO: (yuraz) implement key derivation using openssl
    return nullptr;
  }
}  // namespace libp2p::crypto
