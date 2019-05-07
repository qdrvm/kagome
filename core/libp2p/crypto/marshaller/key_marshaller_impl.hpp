/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_MARSHALER_KEY_MARSHALER_IMPL_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_MARSHALER_KEY_MARSHALER_IMPL_HPP

#include "libp2p/crypto/key_marshaller.hpp"

#include "common/buffer.hpp"
#include "libp2p/crypto/error.hpp"
#include "libp2p/crypto/key.hpp"

namespace libp2p::crypto::marshaller {
  class KeyMarshallerImpl : public KeyMarshaller {
    using Buffer = kagome::common::Buffer;

   public:
    outcome::result<Buffer> marshal(const PublicKey &key) const override;

    outcome::result<Buffer> marshal(const PrivateKey &key) const override;

    outcome::result<PublicKey> unmarshalPublicKey(
        const Buffer &key_bytes) const override;

    outcome::result<PrivateKey> unmarshalPrivateKey(
        const Buffer &key_bytes) const override;
  };
}  // namespace libp2p::crypto::marshaller

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_MARSHALER_KEY_MARSHALER_IMPL_HPP
