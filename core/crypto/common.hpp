/**
* Copyright Quadrivium LLC
* All Rights Reserved
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include <cstdint>
#include <openssl/crypto.h>
#include <boost/assert.hpp>

#include "common/buffer.hpp"

namespace kagome::crypto {

  void cleanse(void* ptr, size_t size);

  template<typename T>
  class SecureHeapAllocator {
   public:
    using value_type = T;
    using pointer = T*;
    using size_type = size_t;

    static pointer allocate(size_type n) {
      BOOST_ASSERT(CRYPTO_secure_malloc_initialized());
      auto p = OPENSSL_secure_malloc(n * sizeof(T));
      if (p == nullptr) throw std::bad_alloc{};
      return reinterpret_cast<T*>(p);
    }

    static void deallocate(pointer p, size_type) noexcept {
      BOOST_ASSERT(CRYPTO_secure_malloc_initialized());
      OPENSSL_secure_free(p);
    }
  };

}
