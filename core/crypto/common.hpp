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

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "log/logger.hpp"

namespace kagome::crypto {

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
    template <std::ranges::contiguous_range R>
      requires std::ranges::output_range<R, T>
    explicit SecureCleanGuard(R &&r) : data{r} {}

    SecureCleanGuard(const SecureCleanGuard &) = delete;
    SecureCleanGuard &operator=(const SecureCleanGuard &) = delete;
    SecureCleanGuard(SecureCleanGuard &&g) noexcept : data{g.data} {
      g.data = {};
    }
    SecureCleanGuard &operator=(SecureCleanGuard &&g) = delete;

    ~SecureCleanGuard() {
      OPENSSL_cleanse(data.data(), data.size_bytes());
    }

    std::span<T, Size> data;
  };

  template <std::ranges::contiguous_range R>
  SecureCleanGuard(R &&r) -> SecureCleanGuard<std::ranges::range_value_t<R>>;

  template <typename T, size_t N>
  SecureCleanGuard(std::array<T, N> &) -> SecureCleanGuard<T, N>;

  template <typename T, size_t N>
  SecureCleanGuard(std::array<T, N> &&) -> SecureCleanGuard<T, N>;

  template <size_t N>
  SecureCleanGuard(common::Blob<N> &) -> SecureCleanGuard<uint8_t, N>;

  template <size_t N>
  SecureCleanGuard(common::Blob<N> &&) -> SecureCleanGuard<uint8_t, N>;

  /**
   * An allocator on the OpenSSL secure heap
   */
  template <typename T, size_t HeapSize = 4096, size_t MinAllocationSize = 32>
  class SecureHeapAllocator {
    inline static std::once_flag flag;
    inline static log::Logger logger;

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
        logger = log::createLogger("SecureAllocator", "crypto");
      });
      BOOST_ASSERT(CRYPTO_secure_malloc_initialized());
      auto p = OPENSSL_secure_malloc(n * sizeof(T));
      SL_TRACE(logger,
               "allocated {} bytes in secure heap, {} used",
               OPENSSL_secure_actual_size(p),
               CRYPTO_secure_used());
      if (p == nullptr) {
        throw std::bad_alloc{};
      }

      return reinterpret_cast<T *>(p);
    }

    static void deallocate(pointer p, size_type) noexcept {
      BOOST_ASSERT(CRYPTO_secure_malloc_initialized());
      OPENSSL_secure_free(p);
    }

    bool operator==(const SecureHeapAllocator &) const = default;
  };

  template <size_t SizeLimit = std::numeric_limits<size_t>::max()>
  using SecureBuffer =
      common::SLBuffer<SizeLimit, SecureHeapAllocator<uint8_t>>;

  /**
   * A container that allocates its data on the OpenSSL secure heap
   * @tparam Size - the key length
   * @tparam Tag - a type-safety tag
   */
  template <size_t Size, typename Tag>
  class PrivateKey {
    template <size_t, typename>
    friend class PrivateKey;

   public:
    PrivateKey() : data(Size, 0) {}

    PrivateKey(const PrivateKey &) = default;
    PrivateKey &operator=(const PrivateKey &) = default;

    PrivateKey(PrivateKey &&key) noexcept = default;

    PrivateKey &operator=(PrivateKey &&key) noexcept = default;

    bool operator==(const PrivateKey &) const noexcept = default;

    template <typename OtherTag>
    bool operator==(const PrivateKey<Size, OtherTag> &key) const noexcept {
      return key == data.view();
    }

    bool operator==(std::span<const uint8_t> bytes) const noexcept {
      return data.view() == bytes;
    }

    static constexpr size_t size() {
      return Size;
    }

    template <size_t OtherSize, typename OtherTag>
      requires(OtherSize >= Size)
    static PrivateKey from(const PrivateKey<OtherSize, OtherTag> &other_key) {
      auto copy = other_key.data;
      return PrivateKey{std::move(copy)};
    }

    template <size_t OtherSize, typename OtherTag>
      requires(OtherSize >= Size)
    static PrivateKey from(PrivateKey<OtherSize, OtherTag> &&other_key) {
      return PrivateKey{std::move(other_key.data)};
    }

    /**
     * SecureCleanGuard ensures that data we used to initialize the key
     * is then immediately erased from its original unsafe storage
     */
    static PrivateKey from(SecureCleanGuard<uint8_t, Size> view) {
      return PrivateKey(view.data);
    }

    /**
     * SecureCleanGuard ensures that data we used to initialize the key
     * is then immediately erased from its original unsafe storage
     */
    static outcome::result<PrivateKey> from(SecureCleanGuard<uint8_t> view) {
      if (view.data.size() != Size) {
        return common::BlobError::INCORRECT_LENGTH;
      }
      return PrivateKey(view.data.subspan<0, Size>());
    }

    template <size_t OtherSize>
      requires(OtherSize >= Size)
    static outcome::result<PrivateKey> from(SecureBuffer<OtherSize> buf) {
      if (buf.size() != Size) {
        return common::BlobError::INCORRECT_LENGTH;
      }
      return PrivateKey(std::move(buf));
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

    static outcome::result<PrivateKey> fromHex(const SecureBuffer<> &hex) {
      OUTCOME_TRY(bytes,
                  common::unhex(std::string_view{
                      reinterpret_cast<const char *>(hex.data()), hex.size()}));
      return PrivateKey::from(SecureCleanGuard<uint8_t>{bytes});
    }

    /**
     * Provides the direct read access to the private key bytes.
     * The bytes copied from here to unsafe memory must later be cleaned up with
     * SecureCleanGuard
     */
    [[nodiscard]] std::span<const uint8_t, Size> unsafeBytes() const {
      return std::span<const uint8_t, Size>(data);
    }

   private:
    explicit PrivateKey(std::span<const uint8_t, Size> view) {
      BOOST_ASSERT(view.size() == Size);
      data.put(view);
    }

    template <size_t OtherSize>
      requires(OtherSize >= Size)
    explicit PrivateKey(SecureBuffer<OtherSize> data)
        : PrivateKey{data.view().template subspan<0, Size>()} {
      BOOST_ASSERT(this->data.size() == Size);
    }

    SecureBuffer<Size> data;
  };
}  // namespace kagome::crypto
