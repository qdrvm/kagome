/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/marshaller/key_marshaller_impl.hpp"

#include "libp2p/crypto/common.hpp"
#include "libp2p/crypto/proto/keys.pb.h"

namespace libp2p::crypto::marshaller {
  using kagome::common::Buffer;
  namespace {
    /**
     * @brief converts Key::Type to proto::KeyType
     * @param key_type common key type value
     * @return proto key type value
     */
    outcome::result<proto::KeyType> marshalKeyType(Key::Type key_type) {
      switch (key_type) {
        case Key::Type::UNSPECIFIED:
          return proto::KeyType::UNSPECIFIED;
        case Key::Type::RSA1024:
          return proto::KeyType::RSA1024;
        case Key::Type::RSA2048:
          return proto::KeyType::RSA2048;
        case Key::Type::RSA4096:
          return proto::KeyType::RSA4096;
        case Key::Type::ED25519:
          return proto::KeyType::ED25519;
        case Key::Type::SECP256K1:
          return proto::KeyType::SECP256K1;
      }

      return CryptoProviderError::UNKNOWN_KEY_TYPE;
    }

    /**
     * @brief converts proto::KeyType to Key::Type
     * @param key_type proto key type value
     * @return common key type value
     */
    outcome::result<Key::Type> unmarshalKeyType(proto::KeyType key_type) {
      switch (key_type) {  // NOLINT
        case proto::KeyType::UNSPECIFIED:
          return Key::Type::UNSPECIFIED;
        case proto::KeyType::RSA1024:
          return Key::Type::RSA1024;
        case proto::KeyType::RSA2048:
          return Key::Type::RSA2048;
        case proto::KeyType::RSA4096:
          return Key::Type::RSA4096;
        case proto::KeyType::ED25519:
          return Key::Type::ED25519;
        case proto::KeyType::SECP256K1:
          return Key::Type::SECP256K1;
      }

      return CryptoProviderError::UNKNOWN_KEY_TYPE;
    }
  }  // namespace

  outcome::result<Buffer> KeyMarshallerImpl::marshal(const PublicKey &key) const {
    proto::PublicKey proto_key;
    OUTCOME_TRY(key_type, marshalKeyType(key.type));
    proto_key.set_key_type(key_type);

    proto_key.set_key_value(key.data.toBytes(), key.data.size());

    auto string = proto_key.SerializeAsString();
    Buffer out;
    out.put(proto_key.SerializeAsString());

    return out;
  }

  outcome::result<Buffer> KeyMarshallerImpl::marshal(const PrivateKey &key) const {
    proto::PublicKey proto_key;
    OUTCOME_TRY(key_type, marshalKeyType(key.type));
    proto_key.set_key_type(key_type);
    proto_key.set_key_value(key.data.toBytes(), key.data.size());

    auto string = proto_key.SerializeAsString();
    Buffer out;
    out.put(proto_key.SerializeAsString());

    return out;
  }

  outcome::result<PublicKey> KeyMarshallerImpl::unmarshalPublicKey(
      const Buffer &key_bytes) const {
    proto::PublicKey proto_key;
    if (!proto_key.ParseFromArray(key_bytes.toBytes(), key_bytes.size())) {
      return CryptoProviderError::FAILED_UNMARSHAL_DATA;
    }

    OUTCOME_TRY(key_type, unmarshalKeyType(proto_key.key_type()));

    Buffer key_value;
    key_value.put(proto_key.key_value());
    return PublicKey{{key_type, key_value}};
  }

  outcome::result<PrivateKey> KeyMarshallerImpl::unmarshalPrivateKey(
      const Buffer &key_bytes) const {
    proto::PublicKey proto_key;
    if (!proto_key.ParseFromArray(key_bytes.toBytes(), key_bytes.size())) {
      return CryptoProviderError::FAILED_UNMARSHAL_DATA;
    }

    OUTCOME_TRY(key_type, unmarshalKeyType(proto_key.key_type()));
    Buffer key_value;
    key_value.put(proto_key.key_value());

    return PrivateKey{{key_type, key_value}};
  }

}  // namespace libp2p::crypto::marshaller
