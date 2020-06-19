/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/secp256k1/secp256k1_provider_impl.hpp"

#include <libsecp256k1/include/secp256k1_recovery.h>

namespace kagome::crypto {

  Secp256k1ProviderImpl::Secp256k1ProviderImpl()
      : context_(secp256k1_context_create(SECP256K1_CONTEXT_SIGN
                                          | SECP256K1_CONTEXT_VERIFY),
                 secp256k1_context_destroy) {}

  outcome::result<Secp256k1UncompressedPublicKey>
  Secp256k1ProviderImpl::recoverPublickeyUncompressed(
      const Secp256k1Signature &signature,
      const Secp256k1Message &message_hash) const {
    OUTCOME_TRY(pubkey, recoverPublickey(signature, message_hash));
    Secp256k1UncompressedPublicKey pubkey_out;
    size_t outputlen = pubkey_out.size();

    if (1
        != secp256k1_ec_pubkey_serialize(context_.get(),
                                         pubkey_out.data(),
                                         &outputlen,
                                         &pubkey,
                                         SECP256K1_EC_UNCOMPRESSED)) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }

    return pubkey_out;
  };

  outcome::result<Secp256k1CompressedPublicKey>
  Secp256k1ProviderImpl::recoverPublickeyCompressed(
      const Secp256k1Signature &signature,
      const Secp256k1Message &message_hash) const {
    OUTCOME_TRY(pubkey, recoverPublickey(signature, message_hash));
    Secp256k1CompressedPublicKey pubkey_out;
    size_t outputlen = pubkey_out.size();

    if (1
        != secp256k1_ec_pubkey_serialize(context_.get(),
                                         pubkey_out.data(),
                                         &outputlen,
                                         &pubkey,
                                         SECP256K1_EC_COMPRESSED)) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }

    return pubkey_out;
  }

  outcome::result<secp256k1_pubkey> Secp256k1ProviderImpl::recoverPublickey(
      const Secp256k1Signature &signature,
      const Secp256k1Message &message_hash) const {
    secp256k1_ecdsa_recoverable_signature sig_rec;
    secp256k1_pubkey pubkey;
    auto v = static_cast<int>(signature[64]);
    int recovery_id = -1;
    // v can be 0/1 27/28
    // recovery_id should be 0 or 1
    switch (v) {
      case 0:
        recovery_id = 0;
        break;
      case 1:
        recovery_id = 1;
        break;
      case 27:
        recovery_id = 0;
        break;
      case 28:
        recovery_id = 1;
        break;
      default:
        return Secp256k1ProviderError::INVALID_ARGUMENT;
    }

    if (1
        != secp256k1_ecdsa_recoverable_signature_parse_compact(
            context_.get(), &sig_rec, signature.data(), recovery_id)) {
      return Secp256k1ProviderError::INVALID_ARGUMENT;
    }

    if (1
        != secp256k1_ecdsa_recover(
            context_.get(), &pubkey, &sig_rec, message_hash.data())) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }

    return pubkey;
  }
}  // namespace kagome::crypto

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, Secp256k1ProviderError, e) {
  using E = kagome::crypto::Secp256k1ProviderError;
  switch (e) {
    case E::INVALID_ARGUMENT:
      return "invalid argument occured";
    case E::RECOVERY_FAILED:
      return "public key recovery operation failed";
  }
  return "unknown Secp256k1ProviderError error occured";
}
