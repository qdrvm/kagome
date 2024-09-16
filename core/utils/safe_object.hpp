/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

#define SAFE_UNIQUE_CAPTURE(x, ...) \
  x ^= __VA_ARGS__(typename std::remove_cvref_t<decltype(x)>::Type & x)
#define SAFE_SHARED_CAPTURE(x, ...) \
  x |= __VA_ARGS__(const typename std::remove_cvref_t<decltype(x)>::Type &x)
#define SAFE_UNIQUE(x) SAFE_UNIQUE_CAPTURE(x, [&])
#define SAFE_SHARED(x) SAFE_SHARED_CAPTURE(x, [&])

// clang-format off
/**
 * Protected object wrapper. Allow read-write access.
 * @tparam T object type
 * Example:
 * @code
 *  SafeObject<std::string> obj("1");
 *  bool const is_one_att1 =
 *      obj.sharedAccess([](auto const &str) {
 *          return str == "1";
 *      });
 *  obj.exclusiveAccess([](auto &str) {
 *      str = "2";
 *  });
 *  bool const is_one_att2 =
 *      obj.sharedAccess([](auto const &str) {
 *          return str == "1";
 *      });
 *
 * std::cout <<
 *   "Attempt 1: " << is_one_att1 << std::endl <<
 *   "Attempt 2: " << is_one_att2;
 * @endcode
 */
// clang-format on
template <typename T, typename M = std::shared_mutex>
struct SafeObject {
  using Type = T;

  template <typename... Args>
  SafeObject(Args &&...args) : t_(std::forward<Args>(args)...) {}

  template <typename F>
  inline auto exclusiveAccess(F &&f) {
    std::unique_lock lock(cs_);
    return std::forward<F>(f)(t_);
  }

  template <typename F>
  inline auto sharedAccess(F &&f) const {
    std::shared_lock lock(cs_);
    return std::forward<F>(f)(t_);
  }

  auto operator^=(auto &&f) {
    return exclusiveAccess(std::forward<decltype(f)>(f));
  }
  auto operator|=(auto &&f) const {
    return sharedAccess(std::forward<decltype(f)>(f));
  }

  T &unsafeGet() {
    return t_;
  }

  const T &unsafeGet() const {
    return t_;
  }

 private:
  T t_;
  mutable M cs_;
};

template <typename T, typename M = std::shared_mutex>
SafeObject(T &&) -> SafeObject<T, M>;

class WaitForSingleObject final {
  std::condition_variable wait_cv_;
  std::mutex wait_m_;
  bool flag_ = true;

 public:
  WaitForSingleObject(const WaitForSingleObject &) = delete;
  WaitForSingleObject &operator=(const WaitForSingleObject &) = delete;

  WaitForSingleObject(WaitForSingleObject &&) = delete;
  WaitForSingleObject &operator=(WaitForSingleObject &&) = delete;

  WaitForSingleObject() = default;
  ~WaitForSingleObject() = default;

  bool wait(std::chrono::microseconds wait_timeout) {
    std::unique_lock<std::mutex> _lock(wait_m_);
    return wait_cv_.wait_for(_lock, wait_timeout, [&]() {
      auto prev = !flag_;
      flag_ = true;
      return prev;
    });
  }

  void wait() {
    std::unique_lock<std::mutex> _lock(wait_m_);
    wait_cv_.wait(_lock, [&]() {
      auto prev = !flag_;
      flag_ = true;
      return prev;
    });
  }

  void set() {
    {
      std::unique_lock<std::mutex> _lock(wait_m_);
      flag_ = false;
    }
    wait_cv_.notify_one();
  }
};
