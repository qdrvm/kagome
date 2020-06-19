/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_SECP256K1_PROVIDER_IMPL_HPP
#define KAGOME_CRYPTO_SECP256K1_PROVIDER_IMPL_HPP

#include <libsecp256k1/include/secp256k1.h>

#include "crypto/secp256k1_provider.hpp"

namespace kagome::crypto {
  class Secp256k1ProviderImpl : public Secp256k1Provider {
   public:
    ~Secp256k1ProviderImpl() override = default;

    outcome::result<Secp256k1UncompressedPublicKey>
    recoverPublickeyUncompressed(
        const Secp256k1Signature &signature,
        const Secp256k1Message &message_hash) const override;

    outcome::result<Secp256k1CompressedPublicKey> recoverPublickeyCompressed(
        const Secp256k1Signature &signature,
        const Secp256k1Message &message_hash) const override;

   private:
    outcome::result<secp256k1_pubkey> recoverPublickey(
        const Secp256k1Signature &signature,
        const Secp256k1Message &message_hash) const;

    std::unique_ptr<secp256k1_context, void (*)(secp256k1_context *)> context_;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_SECP256K1_PROVIDER_IMPL_HPP
