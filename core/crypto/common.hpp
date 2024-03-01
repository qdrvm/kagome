/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <openssl/crypto.h>
#include <boost/assert.hpp>
#include <cstdint>
#include <mutex>
#include <type_traits>

#include "common/buffer.hpp"

namespace kagome::crypto {

  // guaranteed to clean up the memory and not be optimized away
  void secure_cleanup(void *ptr, size_t size);

  /**
   * A wrapper around a span of data
   * that securely cleans up the data when goes out of scope
   */
  template <typename T, size_t Size = std::dynamic_extent>
    requires std::is_standard_layout_v<T>
  struct SecureCleanGuard {
    static_assert(!std::is_const_v<T>,
                  "Secure clean guard must have write access to the data");

    explicit SecureCleanGuard(std::span<T, Size> data) noexcept : data{data} {}
    explicit SecureCleanGuard(std::array<T, Size> &data) noexcept
        : data{data} {}
    template <std::ranges::contiguous_range R>
      requires std::ranges::output_range<R, T>
    explicit SecureCleanGuard(R &&r) : data{r} {}

    SecureCleanGuard(const SecureCleanGuard &) = delete;
    SecureCleanGuard &operator=(const SecureCleanGuard &) = delete;
    SecureCleanGuard(SecureCleanGuard &&g) noexcept : data{g.data} {
      g.data = {};
    }
    SecureCleanGuard &operator=(SecureCleanGuard &&g) noexcept {
      data = g.data;
      g.data = {};
      return *this;
    }

    ~SecureCleanGuard() {
      secure_cleanup(data.data(), data.size_bytes());
    }

    std::span<T, Size> data;
  };

  template <std::ranges::contiguous_range R>
  SecureCleanGuard(R &&r) -> SecureCleanGuard<std::ranges::range_value_t<R>>;

  /**
   * An allocator on the OpenSSL secure heap
   */
  template <typename T, size_t HeapSize = 4096, size_t MinAllocationSize = 32>
  class SecureHeapAllocator {
    inline static std::once_flag flag;

   public:
    using value_type = T;
    using pointer = T *;
    using size_type = size_t;

    template <typename U>
    struct rebind {
      using other = SecureHeapAllocator<U, HeapSize, MinAllocationSize>;
    };

    static pointer allocate(size_type n) {
      std::call_once(flag, []() {
        if (CRYPTO_secure_malloc_init(HeapSize, MinAllocationSize) != 1) {
          throw std::runtime_error{"Failed to allocate OpenSSL secure heap"};
        }
      });
      BOOST_ASSERT(CRYPTO_secure_malloc_initialized());
      auto p = OPENSSL_secure_malloc(n * sizeof(T));
      if (p == nullptr) {
        throw std::bad_alloc{};
      }
      return reinterpret_cast<T *>(p);
    }

    static void deallocate(pointer p, size_type) noexcept {
      BOOST_ASSERT(CRYPTO_secure_malloc_initialized());
      OPENSSL_secure_free(p);
    }
  };

  /**
   * A container that allocates its data on the OpenSSL secure heap
   * @tparam Size - the key length
   * @tparam Tag - a type-safety tag
   */
  template <size_t Size, typename Tag>
  class PrivateKey {
   public:
    PrivateKey() = default;
    PrivateKey(const PrivateKey &) = default;
    PrivateKey &operator=(const PrivateKey &) = default;

    PrivateKey(PrivateKey &&key) noexcept = default;

    PrivateKey &operator=(PrivateKey &&key) noexcept = default;

    bool operator==(const PrivateKey &) const noexcept = default;
    bool operator==(std::span<const uint8_t> bytes) const noexcept {
      return data.view() == bytes;
    }

    static constexpr size_t size() {
      return Size;
    }

    /**
     * SecureCleanGuard ensures that data we used to initialize the key
     * is then immediately erased from its original unsafe storage
     */
    static PrivateKey from(SecureCleanGuard<uint8_t, Size> view) {
      return PrivateKey::from(view.data);
    }

    /**
     * SecureCleanGuard ensures that data we used to initialize the key
     * is then immediately erased from its original unsafe storage
     */
    static outcome::result<PrivateKey> from(SecureCleanGuard<uint8_t> view) {
      if (view.data.size() != Size) {
        return common::BlobError::INCORRECT_LENGTH;
      }
      return PrivateKey::from(view.data.subspan<0, Size>());
    }

    /**
     * SecureCleanGuard ensures that data we used to initialize the key
     * is then immediately erased from its original unsafe storage
     */
    static outcome::result<PrivateKey> fromHex(SecureCleanGuard<char> hex) {
      OUTCOME_TRY(
          bytes,
          common::unhex(std::string_view{hex.data.begin(), hex.data.end()}));
      return PrivateKey::from(SecureCleanGuard<uint8_t>{bytes});
    }

    /**
     * Provides the direct read access to the private key bytes.
     * The bytes copied from here to unsafe memory must later be cleaned up with
     * kagome::crypto::secure_cleanup
     */
    [[nodiscard]] std::span<const uint8_t> unsafeBytes() const {
      return data;
    }

   private:
    static PrivateKey from(std::span<uint8_t, Size> view) {
      PrivateKey key;
      key.data.put(view);
      return key;
    }

    common::SLBuffer<Size, SecureHeapAllocator<uint8_t>> data;
  };
}  // namespace kagome::crypto
