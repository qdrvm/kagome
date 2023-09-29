/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BOX_HPP
#define KAGOME_BOX_HPP

#include <iostream>
#include <type_traits>

template <typename T>
struct Box {
  using Type = T;

  template <typename... A>
  explicit Box(A &&...args) : t_{std::forward<A>(args)...} {}
  Box(Box &box) : Box{std::move(box)} {}

  Box(Box &&box) : t_{std::move(*box.t_)} {
    if constexpr (not std::is_standard_layout_v<T>
                  or not std::is_trivial_v<T>) {
      box.t_ = std::nullopt;
    }
  }

  Box &operator=(Box &val) {
    return operator=(std::move(val));
  }

  Box &operator=(Box &&box) {
    t_ = std::move(*box.t_);
    if constexpr (not std::is_standard_layout_v<T>
                  or not std::is_trivial_v<T>) {
      box.t_ = std::nullopt;
    }
    return *this;
  }

  auto clone() const {
    assert(t_);
    return Box<T>{*t_};
  }

  T &mut_value() & {
    assert(t_);
    return *t_;
  }

  const T &value() const & {
    assert(t_);
    return *t_;
  }

 private:
  std::optional<T> t_;
};

#endif  // KAGOME_BOX_HPP
