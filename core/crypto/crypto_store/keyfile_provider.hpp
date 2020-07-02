/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KEYFILE_PROVIDER_HPP
#define KAGOME_KEYFILE_PROVIDER_HPP

#include <boost/filesystem.hpp>
#include "crypto/crypto_store/key_type.hpp"

namespace kagome::crypto::store {
  class KeyfileProvider {
   public:
    using Path = boost::filesystem::path;

    virtual ~KeyfileProvider() = default;

    virtual bool hasFile(const Path &path) = 0;

    virtual outcome::result<common::Buffer> loadFile(
        KeyTypeId key_type, const ED25519PublicKey &pk) = 0;

    virtual outcome::result<void> storeFile(KeyTypeId key_type,
                                            const ED25519PublicKey &pk,
                                            const common::Buffer &content) = 0;
  };
}  // namespace kagome::crypto::store

#endif  // KAGOME_KEYFILE_PROVIDER_HPP
