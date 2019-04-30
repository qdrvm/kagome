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
    outcome::result<Buffer> encrypt_128_ctr(const Aes128Secret &secret,
                                            const Buffer &data) const override;

    outcome::result<Buffer> decrypt_128_ctr(const Aes128Secret &secret,
                                            const Buffer &data) const override;

    outcome::result<Buffer> encrypt_256_ctr(const Aes256Secret &secret,
                                            const Buffer &data) const override;

    outcome::result<Buffer> decrypt_256_ctr(const Aes256Secret &secret,
                                            const Buffer &data) const override;
  };
}  // namespace libp2p::crypto::aes

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_IMPL_HPP
