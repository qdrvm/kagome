/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/typed_key_storage/typed_key_storage_impl.hpp"
namespace kagome::crypto::storage {

  TypedKeyStorageImpl::Keys TypedKeyStorageImpl::getKeys(KeyTypeId key_type) {
    BOOST_ASSERT_MSG(false, "TypedKeyStorageImpl::getKeys not implemented yet");
    return {};
  }

  void TypedKeyStorageImpl::addKeyPair(KeyPair key_pair) {
    BOOST_ASSERT_MSG(false,
                     "TypedKeyStorageImpl::addKeyPair not implemented yet");
  }
}  // namespace kagome::crypto::storage
