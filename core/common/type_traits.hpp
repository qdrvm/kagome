/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace kagome {

  // SFINAE-way for detect shared pointer
  template <typename T>
  struct is_shared_ptr : std::false_type {};
  template <typename T>
  struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};

  // SFINAE-way for detect unique pointer
  template <typename T>
  struct is_unique_ptr : std::false_type {};
  template <typename T>
  struct is_unique_ptr<std::unique_ptr<T>> : std::true_type {};

  // SFINAE-way for detect unique and smart pointers both
  template <typename T>
  struct is_smart_ptr : std::false_type {};
  template <typename T>
  struct is_smart_ptr<std::unique_ptr<T>> : std::true_type {};
  template <typename T>
  struct is_smart_ptr<std::shared_ptr<T>> : std::true_type {};

}  // namespace kagome
