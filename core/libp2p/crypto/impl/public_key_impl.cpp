/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/impl/public_key_impl.hpp"

namespace libp2p::crypto {

  PublicKeyImpl::PublicKeyImpl(common::KeyType key_type,
                               PublicKeyImpl::Buffer &&bytes)
      : key_type_{key_type}, bytes_(bytes) {}

  PublicKeyImpl::PublicKeyImpl(common::KeyType key_type,
      PublicKeyImpl::Buffer bytes)
      : key_type_{key_type}, bytes_(std::move(bytes)) {}

  common::KeyType PublicKeyImpl::getType() const {
    return key_type_;
  }

  const kagome::common::Buffer &PublicKeyImpl::getBytes() const {
    return bytes_;
  }

  bool PublicKeyImpl::operator==(const Key &other) const {
    return key_type_ == other.getType() && bytes_ == other.getBytes();
  }

}  // namespace libp2p::crypto
