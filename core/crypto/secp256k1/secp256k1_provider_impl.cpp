/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/secp256k1/secp256k1_provider_impl.hpp"

#include "common/blob.hpp"

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/ossl_typ.h>
#include <openssl/rand.h>
#include <openssl/x509.h>

namespace kagome::crypto {
  using common::Hash256;

  // reconstruct public key from a compact signature
  // This is only slightly more CPU intensive than just verifying it.
  // If this function succeeds, the recovered public key is guaranteed to be
  // valid (the signature is a valid signature of the given data for that key)
  //  bool Recover(const Hash256 &hash, const unsigned char *p64, int rec) {
  //    if (rec < 0 || rec >= 3) return false;
  //    ECDSA_SIG *sig = ECDSA_SIG_new();
  //    BN_bin2bn(&p64[0], 32, sig->r);
  //    BN_bin2bn(&p64[32], 32, sig->s);
  //    bool ret = ECDSA_SIG_recover_key_GFp(
  //                   pkey, sig, (unsigned char *)&hash, sizeof(hash), rec, 0)
  //               == 1;
  //    ECDSA_SIG_free(sig);
  //    return ret;
  //  }
  //
  //  bool CPubKey::RecoverCompact(const uint256 &hash,
  //                               const std::vector<unsigned char> &vchSig) {
  //    if (vchSig.size() != 65) {
  //      return false;
  //    }
  //
  //    CECKey key;
  //    if (!key.Recover(hash, &vchSig[1], (vchSig[0] - 27) & ~4)) return false;
  //    key.GetPubKey(*this, (vchSig[0] - 27) & 4);
  //    return true;
  //  }

  using SigR = common::Blob<32u>;
  using SigS = common::Blob<32u>;
  using SigV = uint8_t;

  inline std::tuple<SigR, SigS, SigV> decomposeSignature(
      const Secp256k1Signature &signature) {
    constexpr auto size = SigR::size() + SigS::size() + sizeof(SigV);
    BOOST_ASSERT_MSG(size == signature.size(), "invalid signature size");

    SigR r{};
    SigS s{};
    SigV v{};
    std::copy_n(signature.begin(), SigR::size(), r.begin());
    std::copy_n(signature.begin() + 32, SigS::size(), s.begin());
    v = signature[64];
    return {r, s, v};
  }

  outcome::result<Secp256k1UncompressedPublicKey>
  Secp256k1ProviderImpl::recoverPublickeyUncopressed(
      const Secp256k1Signature &signature,
      const Secp256k1Message &message_hash) const {
    auto [r, s, v] = decomposeSignature(signature);
  };

  outcome::result<Secp256k1CompressedPublicKey>
  Secp256k1ProviderImpl::recoverPublickeyCompressed(
      const Secp256k1Signature &signature,
      const Secp256k1Message &message_hash) const {
    //
  }

  template <class KeyType>
  outcome::result<KeyType> convertEcKeyToBytes(EC_KEY *ec_key,
                                               int (*converter)(EC_KEY *,
                                                                uint8_t **)) {
    KeyType key{};
    // check key size
    int generated_size = converter(ec_key, nullptr);
    if (generated_size != key.size()) {
      return Secp256k1ProviderError::INVALID_ARGUMENT;
    }
    uint8_t *key_ptr = key.data();
    generated_size = converter(ec_key, &key_ptr);
    if (generated_size != key.size()) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }
    return key;
  }

  // Perform ECDSA key recovery (see SEC1 4.1.6) for curves over (mod p)-fields
  // recid selects which key is recovered
  // if do_additional_check is nonzero, additional checks are performed
  outcome::result<Secp256k1UncompressedPublicKey> ECDSA_SIG_recover_key_GFp(
      const Secp256k1Signature &signature,
      gsl::span<const uint8_t> msg,
      int recid,
      bool do_additional_check) {
    BN_CTX *ctx = nullptr;
    auto free_bn_ctx = gsl::finally([ctx] {
      if (ctx != nullptr) {
        BN_CTX_end(ctx);
        BN_CTX_free(ctx);
      };
    });

    BIGNUM *x = nullptr;
    BIGNUM *e = nullptr;
    BIGNUM *order = nullptr;
    BIGNUM *sor = nullptr;
    BIGNUM *eor = nullptr;
    BIGNUM *field = nullptr;
    BIGNUM *rr = nullptr;
    BIGNUM *zero = nullptr;

    EC_POINT *R = nullptr;
    EC_POINT *O = nullptr;
    EC_POINT *Q = nullptr;
    auto free_roq = gsl::finally([R, O, Q] {
      if (nullptr != R) EC_POINT_free(R);
      if (O != nullptr) EC_POINT_free(O);
      if (Q != nullptr) EC_POINT_free(Q);
    });

    int n = 0;
    int i = recid / 2;

    EC_KEY *eckey = nullptr;
    auto free_ec_key = gsl::finally([eckey] {
      if (eckey != nullptr) EC_KEY_free(eckey);
    });
    eckey = EC_KEY_new();

    const EC_GROUP *group = EC_KEY_get0_group(eckey);
    if (nullptr == (ctx = BN_CTX_new())) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }

    BN_CTX_start(ctx);
    order = BN_CTX_get(ctx);
    if (1 != EC_GROUP_get_order(group, order, ctx)) {
      return Secp256k1ProviderError::INVALID_ARGUMENT;
    }
    x = BN_CTX_get(ctx);
    if (nullptr == BN_copy(x, order)) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }
    if (1 != BN_mul_word(x, i)) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }
    auto [r, s, v] = decomposeSignature(signature);
    BIGNUM *sig_r = BN_new();
    BIGNUM *sig_s = BN_new();
    auto free_sig_r = gsl::finally([sig_r] {
      if (sig_r != nullptr) BN_free(sig_r);
    });
    auto free_sig_s = gsl::finally([sig_s] {
      if (sig_s != nullptr) BN_free(sig_s);
    });
    BN_bin2bn(r.data(), decltype(r)::size(), sig_r);
    BN_bin2bn(s.data(), decltype(s)::size(), sig_s);
    if (1 != BN_add(x, x, sig_r)) {
      return Secp256k1ProviderError::INVALID_ARGUMENT;
    }
    field = BN_CTX_get(ctx);
    if (1 != EC_GROUP_get_curve_GFp(group, field, nullptr, nullptr, ctx)) {
      return Secp256k1ProviderError::INVALID_ARGUMENT;
    }
    if (BN_cmp(x, field) >= 0) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }
    if ((R = EC_POINT_new(group)) == nullptr) {
      return Secp256k1ProviderError::INVALID_ARGUMENT;
    }
    if (1
        != EC_POINT_set_compressed_coordinates_GFp(
            group, R, x, recid % 2, ctx)) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }

    if (do_additional_check) {
      if (nullptr == (O = EC_POINT_new(group))) {
        return Secp256k1ProviderError::RECOVERY_FAILED;
      }
      if (1 != EC_POINT_mul(group, O, nullptr, R, order, ctx)) {
        return Secp256k1ProviderError::RECOVERY_FAILED;
      }
      if (1 != EC_POINT_is_at_infinity(group, O)) {
        return Secp256k1ProviderError::INVALID_ARGUMENT;
      }
    }
    if (nullptr == (Q = EC_POINT_new(group))) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }

    n = EC_GROUP_get_degree(group);
    e = BN_CTX_get(ctx);
    if (nullptr == BN_bin2bn(msg.data(), msg.size(), e)) {
      return Secp256k1ProviderError::INVALID_ARGUMENT;
    }

    if (8 * msg.size() > n) {
      BN_rshift(e, e, 8 - (n & 7));
    }
    zero = BN_CTX_get(ctx);
    if (1 != BN_zero(zero)) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }

    if (1 != BN_mod_sub(e, zero, e, order, ctx)) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }

    rr = BN_CTX_get(ctx);
    if (nullptr == BN_mod_inverse(rr, sig_r, order, ctx)) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }
    sor = BN_CTX_get(ctx);
    if (1 != BN_mod_mul(sor, sig_s, rr, order, ctx)) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }

    eor = BN_CTX_get(ctx);
    if (1 != BN_mod_mul(eor, e, rr, order, ctx)) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }

    if (1 != EC_POINT_mul(group, Q, eor, R, sor, ctx)) {
      return Secp256k1ProviderError::INVALID_ARGUMENT;
    }
    if (1 != EC_KEY_set_public_key(eckey, Q)) {
      return Secp256k1ProviderError::RECOVERY_FAILED;
    }

    return convertEcKeyToBytes<Secp256k1UncompressedPublicKey>(eckey,
                                                               i2d_EC_PUBKEY);
  }

}  // namespace kagome::crypto

OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, Secp256k1ProviderError, e) {
  using E = kagome::crypto::Secp256k1ProviderError;
  switch (e) {
    case E::INVALID_ARGUMENT:
      return "invalid argument occured";
    case E::RECOVERY_FAILED:
      "public key recovery operation failed";
  }
  return "unknown Secp256k1ProviderError error occured";
}
