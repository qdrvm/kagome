/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_TESTUTIL_LAZY_HPP_
#define KAGOME_TEST_TESTUTIL_LAZY_HPP_

#include <memory>

#include "injector/lazy.hpp"

namespace testutil {

  template <typename T>
  struct CreatorSptr {
    template <typename R>
    R create() {
      return std::static_pointer_cast<typename R::element_type>(
          reinterpret_cast<T &>(*this));
    }
  };

  template <typename T, typename A = T>
  auto sptr_to_lazy(std::shared_ptr<A> &arg) {
    return kagome::LazySPtr<T>(
        reinterpret_cast<CreatorSptr<std::shared_ptr<A>> &>(arg));
  }

}  // namespace testutil

#endif  // KAGOME_TEST_TESTUTIL_LAZY_HPP_
