/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/marshaller/key_marshaller_impl.hpp"

#include "libp2p/crypto/common.hpp"
#include "libp2p/crypto/key_generator.hpp"
#include "libp2p/crypto/proto/keys.pb.h"

namespace libp2p::crypto::marshaller {
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
      switch (key_type) {
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
        default:
          return CryptoProviderError::UNKNOWN_KEY_TYPE;
      }
    }
  }  // namespace

  KeyMarshallerImpl::KeyMarshallerImpl(
      std::shared_ptr<validator::KeyValidator> key_validator)
      : key_validator_{std::move(key_validator)} {}

  outcome::result<KeyMarshallerImpl::ByteArray> KeyMarshallerImpl::marshal(
      const PublicKey &key) const {
    proto::PublicKey proto_key;
    OUTCOME_TRY(key_type, marshalKeyType(key.type));
    proto_key.set_key_type(key_type);
    proto_key.set_key_value(key.data.data(), key.data.size());

    auto string = proto_key.SerializeAsString();
    KeyMarshallerImpl::ByteArray out(string.begin(), string.end());
    return KeyMarshallerImpl::ByteArray(string.begin(), string.end());
  }

  outcome::result<KeyMarshallerImpl::ByteArray> KeyMarshallerImpl::marshal(
      const PrivateKey &key) const {
    proto::PublicKey proto_key;
    OUTCOME_TRY(key_type, marshalKeyType(key.type));
    proto_key.set_key_type(key_type);
    proto_key.set_key_value(key.data.data(), key.data.size());

    auto string = proto_key.SerializeAsString();
    return KeyMarshallerImpl::ByteArray(string.begin(), string.end());
  }

  outcome::result<PublicKey> KeyMarshallerImpl::unmarshalPublicKey(
      const KeyMarshallerImpl::ByteArray &key_bytes) const {
    proto::PublicKey proto_key;
    if (!proto_key.ParseFromArray(key_bytes.data(), key_bytes.size())) {
      return CryptoProviderError::FAILED_UNMARSHAL_DATA;
    }

    OUTCOME_TRY(key_type, unmarshalKeyType(proto_key.key_type()));
    KeyMarshallerImpl::ByteArray key_value(proto_key.key_value().begin(),
                                           proto_key.key_value().end());
    auto key = PublicKey{{key_type, key_value}};
    OUTCOME_TRY(key_validator_->validate(key));

    return key;
  }

  outcome::result<PrivateKey> KeyMarshallerImpl::unmarshalPrivateKey(
      const KeyMarshallerImpl::ByteArray &key_bytes) const {
    proto::PublicKey proto_key;
    if (!proto_key.ParseFromArray(key_bytes.data(), key_bytes.size())) {
      return CryptoProviderError::FAILED_UNMARSHAL_DATA;
    }

    OUTCOME_TRY(key_type, unmarshalKeyType(proto_key.key_type()));
    KeyMarshallerImpl::ByteArray key_value(proto_key.key_value().begin(),
                                           proto_key.key_value().end());
    auto key = PrivateKey{{key_type, key_value}};
    OUTCOME_TRY(key_validator_->validate(key));

    return key;
  }

}  // namespace libp2p::crypto::marshaller
