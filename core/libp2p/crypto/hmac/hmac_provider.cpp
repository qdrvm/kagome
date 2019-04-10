/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/hmac/hmac_provider.hpp"

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "libp2p/crypto/error.hpp"

namespace libp2p::crypto::hmac {
  using Buffer = kagome::common::Buffer;

  int HmacProvider::digestSize(common::HashType type) {
    switch (type) {
      case common::HashType::kSHA1:
        return 20;
      case common::HashType::kSHA256:
        return 32;
      case common::HashType::kSHA512:
        return 64;
      default:
        break;
    }
    return 0;
  }

  const evp_md_st *makeHashTraits(common::HashType type) {
    switch (type) {
      case common::HashType::kSHA1:
        return EVP_sha1();
      case common::HashType::kSHA256:
        return EVP_sha256();
      case common::HashType::kSHA512:
        return EVP_sha512();
      default:
        break;
    }
    return nullptr;
  }

  outcome::result<Buffer> HmacProvider::makeDigest(HashType hash,
                                                   const Buffer &secret,
                                                   const Buffer &data) const {
    //    uint8_t *digest = HMAC(evp_md, secret.toBytes(), secret.size(),
    //                           data.toBytes(), data.size(), nullptr, nullptr);
    //
    //    Buffer result;
    //    result.put(gsl::span(digest, digest_size));
    //
    //    // it seems that returned value is placed on stack
    //    // or managed by openssl library
    //
    //    return result;

    const auto *evp_md = makeHashTraits(hash);
    int digest_size = digestSize(hash);
    if (evp_md == nullptr || digest_size == 0) {
      return HmacProviderError::kUnsupportedHashMethod;
    }

    std::vector<uint8_t> result;
    result.resize(digest_size);
    std::fill(result.begin(), result.end(), 0);

    auto *ctx = EVP_MD_CTX_create();

    auto clean_ctx_at_exit = gsl::finally([ctx]() {
      if (ctx != nullptr) {
        EVP_MD_CTX_destroy(ctx);
      }
    });

    if (1 != EVP_DigestInit_ex(ctx, evp_md, nullptr)) {
      return OpenSslError::kFailedInitializeContext;
    }

    if (1 != EVP_DigestUpdate(ctx, data.toBytes(), data.size())) {
      return OpenSslError::kFailedEncryptUpdate;
    }

    unsigned int len = 0;
    if (1 != EVP_DigestFinal_ex(ctx, result.data(), &len)) {
      return OpenSslError::kFailedEncryptFinalize;
    }

    assert(len == digest_size && "digest size is incorrect");

    return Buffer(std::move(result));
  }
}  // namespace libp2p::crypto::hmac
