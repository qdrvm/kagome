/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/hmac/hmac_provider.hpp"

#include <openssl/hmac.h>
#include "libp2p/crypto/error/error.hpp"

namespace libp2p::crypto::hmac {
  using Buffer = kagome::common::Buffer;

  int digestSize(common::HashType type) {
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

  outcome::result<Buffer> HmacProvider::calculateDigest(
      HashType hash_type, const Buffer &key, const Buffer &message) const {
    const evp_md_st *evp_md = makeHashTraits(hash_type);
    int digest_size = digestSize(hash_type);
    if (evp_md == nullptr || digest_size == 0) {
      return HmacProviderError::UNSUPPORTED_HASH_METHOD;
    }

    std::vector<uint8_t> result(digest_size, 0);
    unsigned int len = 0;

    hmac_ctx_st *ctx = HMAC_CTX_new();
    if (nullptr == ctx) {
      return HmacProviderError::FAILED_CREATE_CONTEXT;
    }

    auto clean_ctx_at_exit = gsl::finally([ctx]() { HMAC_CTX_free(ctx); });

    if (1 != HMAC_Init_ex(ctx, key.toBytes(), key.size(), evp_md, nullptr)) {
      return HmacProviderError::FAILED_INITIALIZE_CONTEXT;
    }

    if (1 != HMAC_Update(ctx, message.toBytes(), message.size())) {
      return HmacProviderError::FAILED_UPDATE_DIGEST;
    }

    if (1 != HMAC_Final(ctx, result.data(), &len)) {
      return HmacProviderError::FAILED_FINALIZE_DIGEST;
    }

    if (digest_size != len) {
      return HmacProviderError::WRONG_DIGEST_SIZE;
    }

    return Buffer(std::move(result));
  }
}  // namespace libp2p::crypto::hmac
