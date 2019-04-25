/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_MARSHALER_KEY_MARSHALER_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_MARSHALER_KEY_MARSHALER_HPP

#include "common/buffer.hpp"
#include "libp2p/crypto/error.hpp"
#include "libp2p/crypto/private_key.hpp"
#include "libp2p/crypto/public_key.hpp"

namespace libp2p::crypto::marshaller {
  /**
   * @class KeyMarshaller provides methods for serializing and deserializing
   * private and public keys from/to google-protobuf format
   */
  class KeyMarshaller {
    using Buffer = kagome::common::Buffer;

   public:
    /**
     * Convert the public key into Protobuf representation
     * @param key - public key to be mashalled
     * @return bytes of Protobuf object if marshalling was successful, error
     * otherwise
     */
    outcome::result<Buffer> marshal(const PublicKey &key) const;

    /**
     * Convert the private key into Protobuf representation
     * @param key - public key to be mashalled
     * @return bytes of Protobuf object if marshalling was successful, error
     * otherwise
     */
    outcome::result<Buffer> marshal(const PrivateKey &key) const;

    /**
     * Convert Protobuf representation of public key into the object
     * @param key_bytes - bytes of the public key
     * @return public key in case of success, error otherwise
     */
    outcome::result<PublicKey> unmarshalPublicKey(
        const Buffer &key_bytes) const;

    /**
     * Convert Protobuf representation of private key into the object
     * @param key_bytes - bytes of the private key
     * @return private key in case of success, error otherwise
     */
    outcome::result<PrivateKey> unmarshalPrivateKey(
        const Buffer &key_bytes) const;
  };
}  // namespace libp2p::crypto::marshaller

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_MARSHALER_KEY_MARSHALER_HPP
