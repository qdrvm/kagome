/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/marshaller/key_marshaller.hpp"

#include "libp2p/crypto/common.hpp"
#include "libp2p/crypto/proto/keys.pb.h"

namespace libp2p::crypto::marshaller {
  using kagome::common::Buffer;
  namespace {
    /**
     * @brief converts common::KeyType to proto::KeyType
     * @param key_type common key type value
     * @return proto key type value
     */
    outcome::result<proto::KeyType> marshalKeyType(common::KeyType key_type) {
      switch (key_type) {
        case common::KeyType::UNSPECIFIED:
          return proto::KeyType::UNSPECIFIED;
        case common::KeyType::RSA1024:
          return proto::KeyType::RSA1024;
        case common::KeyType::RSA2048:
          return proto::KeyType::RSA2048;
        case common::KeyType::RSA4096:
          return proto::KeyType::RSA4096;
        case common::KeyType::ED25519:
          return proto::KeyType::ED25519;
        case common::KeyType::SECP256K1:
          return proto::KeyType::SECP256K1;
      }
      return CryptoProviderError::UNKNOWN_KEY_TYPE;
    }

    /**
     * @brief converts proto::KeyType to common::KeyType
     * @param key_type proto key type value
     * @return common key type value
     */
    outcome::result<common::KeyType> unmarshalKeyType(proto::KeyType key_type) {
      switch (key_type) {  // NOLINT
        case proto::KeyType::UNSPECIFIED:
          return common::KeyType::UNSPECIFIED;
        case proto::KeyType::RSA1024:
          return common::KeyType::RSA1024;
        case proto::KeyType::RSA2048:
          return common::KeyType::RSA2048;
        case proto::KeyType::RSA4096:
          return common::KeyType::RSA4096;
        case proto::KeyType::ED25519:
          return common::KeyType::ED25519;
        case proto::KeyType::SECP256K1:
          return common::KeyType::SECP256K1;
      }
      return CryptoProviderError::UNKNOWN_KEY_TYPE;
    }
  }  // namespace

  outcome::result<Buffer> KeyMarshaller::marshal(const PublicKey &key) const {
    proto::PublicKey proto_key;
    OUTCOME_TRY(key_type, marshalKeyType(key.getType()));
    proto_key.set_key_type(key_type);

    proto_key.set_key_value(key.getBytes().toBytes(), key.getBytes().size());

    auto string = proto_key.SerializeAsString();
    Buffer out;
    out.put(proto_key.SerializeAsString());

    return out;
  }

  outcome::result<Buffer> KeyMarshaller::marshal(const PrivateKey &key) const {
    proto::PublicKey proto_key;
    OUTCOME_TRY(key_type, marshalKeyType(key.getType()));
    proto_key.set_key_type(key_type);
    proto_key.set_key_value(key.getBytes().toBytes(), key.getBytes().size());

    auto string = proto_key.SerializeAsString();
    Buffer out;
    out.put(proto_key.SerializeAsString());

    return out;
  }

  outcome::result<PublicKey> KeyMarshaller::unmarshalPublicKey(
      const Buffer &key_bytes) const {
    proto::PublicKey proto_key;
    if (!proto_key.ParseFromArray(key_bytes.toBytes(), key_bytes.size())) {
      return CryptoProviderError::FAILED_UNMARSHAL_DATA;
    }

    OUTCOME_TRY(key_type, unmarshalKeyType(proto_key.key_type()));

    Buffer key_value;
    key_value.put(proto_key.key_value());
    return PublicKey(key_type, key_value);
  }

  outcome::result<PrivateKey> KeyMarshaller::unmarshalPrivateKey(
      const Buffer &key_bytes) const {
    proto::PublicKey proto_key;
    if (!proto_key.ParseFromArray(key_bytes.toBytes(), key_bytes.size())) {
      return CryptoProviderError::FAILED_UNMARSHAL_DATA;
    }

    OUTCOME_TRY(key_type, unmarshalKeyType(proto_key.key_type()));
    Buffer key_value;
    key_value.put(proto_key.key_value());

    return PrivateKey(key_type, key_value);
  }

}  // namespace libp2p::crypto::marshaller
