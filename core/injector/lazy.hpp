/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <boost/di/extension/injections/lazy.hpp>

namespace kagome {

  template <typename T>
  using Lazy = boost::di::extension::lazy<T>;

  template <typename T>
  using LazyRef = boost::di::extension::lazy<T &>;

  template <typename T>
  using LazyCRef = boost::di::extension::lazy<const T &>;

  template <typename T>
  using LazySPtr = boost::di::extension::lazy<std::shared_ptr<T>>;

  template <typename T>
  using LazyUPtr = boost::di::extension::lazy<std::unique_ptr<T>>;

}  // namespace kagome
