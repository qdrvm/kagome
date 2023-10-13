/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

namespace kagome {
  /**
   * `bool operator==(std::weak_ptr<T>, std::weak_ptr<T>)`.
   */
  template <typename T>
  bool wptrEq(const std::weak_ptr<T> &l, const std::weak_ptr<T> &r) {
    return not l.owner_before(r) and not r.owner_before(l);
  }

  /**
   * `wptrEmpty(std::weak_ptr{}) == true`.
   * `wptrEmpty(expired) == false`.
   */
  template <typename T>
  bool wptrEmpty(const std::weak_ptr<T> &w) {
    return wptrEq(w, {});
  }

  /**
   * Assert that `std::weak_ptr` is not expired.
   */
  template <typename T>
  std::shared_ptr<T> wptrLock(const std::weak_ptr<T> &w) {
    auto s = w.lock();
    [[unlikely]] if (not s and not wptrEmpty(w)) { throw std::bad_weak_ptr{}; }
    return s;
  }

  /**
   * Assert that `std::weak_ptr` is neither empty nor expired.
   */
  template <typename T>
  std::shared_ptr<T> wptrMustLock(const std::weak_ptr<T> &w) {
    auto s = w.lock();
    [[unlikely]] if (not s) { throw std::bad_weak_ptr{}; }
    return s;
  }
}  // namespace kagome
