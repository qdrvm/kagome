/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/typed_key_storage/typed_key_storage_impl.hpp"
namespace kagome::crypto::storage {

  // It's not clear yet what keys should be returned
  TypedKeyStorageImpl::Keys TypedKeyStorageImpl::getEdKeys(KeyTypeId key_type) {
    BOOST_ASSERT_MSG(false, "TypedKeyStorageImpl::getKeys not implemented yet");
    return {};
  }

  TypedKeyStorageImpl::Keys TypedKeyStorageImpl::getSrKeys(KeyTypeId key_type) {
    BOOST_ASSERT_MSG(false, "TypedKeyStorageImpl::getKeys not implemented yet");
    return {};
  }

  // these methods are implemented to enable other crypto api methods,
  // which depend on putting keys into storage
  // this implementation is a stub
  // probably final implementation will differ
  void TypedKeyStorageImpl::addEdKeyPair(KeyTypeId key_type, KeyPair key_pair) {
    ed_keys_.insert({key_type, std::move(key_pair)});
  }

  void TypedKeyStorageImpl::addSrKeyPair(KeyTypeId key_type, KeyPair key_pair) {
    sr_keys_.insert({key_type, std::move(key_pair)});
  }

}  // namespace kagome::crypto::storage
