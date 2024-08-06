/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <type_traits>

#include <boost/assert.hpp>

namespace kagome {
  template <typename T>
  class OptRef {
   public:
    OptRef() : data{nullptr} {}
    OptRef(T &data) : data{&data} {}
    OptRef(T &&) = delete;
    OptRef(std::nullopt_t) : data{nullptr} {}

    OptRef(const OptRef &) = default;

    OptRef &operator=(const OptRef &) = default;

    T &operator*() {
      BOOST_ASSERT(data);
      return *data;
    }

    const T &operator*() const {
      BOOST_ASSERT(data);
      return *data;
    }

    T *operator->() {
      BOOST_ASSERT(data);
      return data;
    }

    const T *operator->() const {
      BOOST_ASSERT(data);
      return data;
    }

    T &value() {
      BOOST_ASSERT(data);
      return *data;
    }

    const T &value() const {
      BOOST_ASSERT(data);
      return *data;
    }

    explicit operator bool() const {
      return data != nullptr;
    }

    bool operator!() const {
      return data == nullptr;
    }

    bool has_value() const {
      return data != nullptr;
    }

    bool operator==(const OptRef<T> &) const = default;

    bool operator==(const T &other) const {
      return has_value() && (*data == other);
    }

   private:
    T *data;
  };
}  // namespace kagome
