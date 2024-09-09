/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/ecdsa/ecdsa_provider_impl.hpp"

#include <secp256k1_recovery.h>

#include "crypto/hasher.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, EcdsaProviderImpl::Error, e) {
  using E = decltype(e);
  switch (e) {
    case E::VERIFICATION_FAILED:
      return "Internal error during ecdsa signature verification";
    case E::SIGN_FAILED:
      return "Internal error during ecdsa signing";
    case E::SOFT_JUNCTION_NOT_SUPPORTED:
      return "Soft junction not supported for ecdsa";
    case E::DERIVE_FAILED:
      return "Internal error during ecdsa deriving";
  }
  return "Unknown error in ecdsa provider";
}

namespace kagome::crypto {
  EcdsaProviderImpl::EcdsaProviderImpl(std::shared_ptr<Hasher> hasher)
      : context_{
          secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY),
          secp256k1_context_destroy,
        },
        hasher_{std::move(hasher)},
        logger_{log::createLogger("EcdsaProvider", "ecdsa")} {}

  outcome::result<EcdsaKeypair> EcdsaProviderImpl::generateKeypair(
      const EcdsaSeed &_seed, Junctions junctions) const {
    auto seed = _seed;
    for (auto &junction : junctions) {
      if (not junction.hard) {
        return Error::SOFT_JUNCTION_NOT_SUPPORTED;
      }
      auto bytes =
          scale::encode("Secp256k1HDKD"_bytes, seed.unsafeBytes(), junction.cc)
              .value();
      SecureCleanGuard g{bytes};
      auto _ = hasher_->blake2b_256(bytes);
      seed = EcdsaSeed::from(
          SecureCleanGuard(static_cast<std::array<uint8_t, 32> &>(_)));
    }
    EcdsaKeypair keys{
        .secret_key = EcdsaPrivateKey::from(seed),
        .public_key = {},
    };
    secp256k1_pubkey ffi_pub;
    if (secp256k1_ec_pubkey_create(
            context_.get(), &ffi_pub, keys.secret_key.unsafeBytes().data())
        == 0) {
      return Error::DERIVE_FAILED;
    }
    size_t size = EcdsaPublicKey::size();
    if (secp256k1_ec_pubkey_serialize(context_.get(),
                                      keys.public_key.data(),
                                      &size,
                                      &ffi_pub,
                                      SECP256K1_EC_COMPRESSED)
        == 0) {
      return Error::DERIVE_FAILED;
    }
    return keys;
  }

  outcome::result<EcdsaSignature> EcdsaProviderImpl::sign(
      common::BufferView message, const EcdsaPrivateKey &key) const {
    return signPrehashed(hasher_->blake2b_256(message), key);
  }

  outcome::result<EcdsaSignature> EcdsaProviderImpl::signPrehashed(
      const EcdsaPrehashedMessage &message, const EcdsaPrivateKey &key) const {
    secp256k1_ecdsa_recoverable_signature ffi_sig;
    if (secp256k1_ecdsa_sign_recoverable(context_.get(),
                                         &ffi_sig,
                                         message.data(),
                                         key.unsafeBytes().data(),
                                         secp256k1_nonce_function_rfc6979,
                                         nullptr)
        == 0) {
      return Error::SIGN_FAILED;
    }
    EcdsaSignature sig{};
    int recid = 0;
    if (secp256k1_ecdsa_recoverable_signature_serialize_compact(
            context_.get(), sig.data(), &recid, &ffi_sig)
        == 0) {
      return Error::SIGN_FAILED;
    }
    sig[64] = static_cast<uint8_t>(recid);
    return sig;
  }

  outcome::result<bool> EcdsaProviderImpl::verify(
      common::BufferView message,
      const EcdsaSignature &signature,
      const EcdsaPublicKey &publicKey,
      bool allow_overflow) const {
    return verifyPrehashed(
        hasher_->blake2b_256(message), signature, publicKey, allow_overflow);
  }

  outcome::result<bool> EcdsaProviderImpl::verifyPrehashed(
      const EcdsaPrehashedMessage &message,
      const EcdsaSignature &signature,
      const EcdsaPublicKey &publicKey) const {
    return verifyPrehashed(message, signature, publicKey, false);
  }

  outcome::result<bool> EcdsaProviderImpl::verifyPrehashed(
      const EcdsaPrehashedMessage &message,
      const EcdsaSignature &signature,
      const EcdsaPublicKey &publicKey,
      bool allow_overflow) const {
    OUTCOME_TRY(recovered,
                recovery_.recoverPublickeyCompressed(
                    signature, message, allow_overflow));
    return recovered == publicKey;
  }
}  // namespace kagome::crypto
