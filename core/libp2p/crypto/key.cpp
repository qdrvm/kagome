/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/key.hpp"

namespace libp2p::crypto {

  Key::Key(common::KeyType key_type, Key::Buffer &&bytes)
      : key_type_{key_type}, bytes_(bytes) {}

  Key::Key(common::KeyType key_type, Key::Buffer bytes)
      : key_type_{key_type}, bytes_(std::move(bytes)) {}

  common::KeyType Key::getType() const {
    return key_type_;
  }

  const kagome::common::Buffer &Key::getBytes() const {
    return bytes_;
  }

  bool Key::operator==(const Key &other) const {
    if (typeid(other) != typeid(*this)) {
      return false;
    }

    if (getType() != other.getType()) {
      return false;
    }

    return getBytes() == other.getBytes();
  }

}  // namespace libp2p::crypto
