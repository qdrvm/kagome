/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_IMPL_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_IMPL_HPP

#include "libp2p/crypto/aes_provider.hpp"

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/crypto/common.hpp"

namespace libp2p::crypto::aes {
  class AesProviderImpl : public AesProvider {
    using Buffer = kagome::common::Buffer;
    using Aes128Secret = libp2p::crypto::common::Aes128Secret;
    using Aes256Secret = libp2p::crypto::common::Aes256Secret;

   public:
    outcome::result<Buffer> encryptAesCtr128(const Aes128Secret &secret,
                                             const Buffer &data) const override;

    outcome::result<Buffer> decryptAesCtr128(const Aes128Secret &secret,
                                             const Buffer &data) const override;

    outcome::result<Buffer> encryptAesCtr256(const Aes256Secret &secret,
                                             const Buffer &data) const override;

    outcome::result<Buffer> decryptAesCtr256(const Aes256Secret &secret,
                                             const Buffer &data) const override;
  };
}  // namespace libp2p::crypto::aes

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_IMPL_HPP
