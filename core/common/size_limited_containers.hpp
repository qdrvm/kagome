/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fmt/format.h>
#include <gsl/span>
#include <limits>
#include <soralog/common.hpp>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "cxx20/lexicographical_compare_three_way.hpp"

namespace kagome::common {

  class MaxSizeException : public std::length_error {
   public:
    template <typename Format, typename... Args>
    MaxSizeException(const Format &format, Args &&...args)
        : std::length_error(
            ::fmt::vformat(format, ::fmt::make_format_args(args...))) {}
  };

  template <typename BaseContainer, std::size_t MaxSize>
  class SizeLimitedContainer : public BaseContainer {
   protected:
    using Base = BaseContainer;
    using Span = gsl::span<typename Base::value_type>;

    static constexpr bool size_check_is_enabled =
        MaxSize < std::numeric_limits<typename Base::size_type>::max();

   public:
    // Next line is required at least for the scale-codec
    static constexpr bool is_static_collection = false;

    [[nodiscard]] inline constexpr typename Base::size_type max_size() {
      return MaxSize;
    }

    SizeLimitedContainer() = default;

    explicit SizeLimitedContainer(size_t size)
        : Base([&] {
            if constexpr (size_check_is_enabled) {
              unlikely_if(size > max_size()) {
                throw MaxSizeException(
                    "Destination has limited size by {}; requested size is {}",
                    max_size(),
                    size);
              }
            }
            return Base(size);
          }()) {}

    SizeLimitedContainer(size_t size, const typename Base::value_type &value)
        : Base([&] {
            if constexpr (size_check_is_enabled) {
              unlikely_if(size > max_size()) {
                throw MaxSizeException(
                    "Destination has limited size by {}; requested size is {}",
                    max_size(),
                    size);
              }
            }
            return Base(size, value);
          }()) {}

    SizeLimitedContainer(const Base &other)
        : Base([&] {
            if constexpr (size_check_is_enabled) {
              unlikely_if(other.size() > max_size()) {
                throw MaxSizeException(
                    "Destination has limited size by {}; Source size is {}",
                    max_size(),
                    other.size());
              }
            }
            return other;
          }()) {}

    SizeLimitedContainer(Base &&other)
        : Base([&] {
            if constexpr (size_check_is_enabled) {
              unlikely_if(other.size() > max_size()) {
                throw MaxSizeException(
                    "Destination has limited size by {}; Source size is {}",
                    max_size(),
                    other.size());
              }
            }
            return std::move(other);
          }()) {}

    template <typename Iter,
              typename = std::enable_if_t<std::is_base_of_v<
                  std::input_iterator_tag,
                  typename std::iterator_traits<Iter>::iterator_category>>>
    SizeLimitedContainer(Iter begin, Iter end)
        : Base([&] {
            if constexpr (size_check_is_enabled) {
              const size_t size = std::distance(begin, end);
              unlikely_if(size > max_size()) {
                throw MaxSizeException(
                    "Container has limited size by {}; Source range size is {}",
                    max_size(),
                    Base::size());
              }
            }
            return Base(std::move(begin), std::move(end));
          }()) {}

    SizeLimitedContainer(std::initializer_list<typename Base::value_type> list)
        : SizeLimitedContainer(list.begin(), list.end()) {}

    SizeLimitedContainer &operator=(const Base &other) {
      if constexpr (size_check_is_enabled) {
        unlikely_if(other.size() > max_size()) {
          throw MaxSizeException(
              "Destination has limited size by {}; Source size is {}",
              max_size(),
              other.size());
        }
      }
      static_cast<Base &>(*this) = other;
      return *this;
    }

    SizeLimitedContainer &operator=(Base &&other) {
      if constexpr (size_check_is_enabled) {
        unlikely_if(other.size() > max_size()) {
          throw MaxSizeException(
              "Destination has limited size by {}; Source size is {}",
              max_size(),
              other.size());
        }
      }
      static_cast<Base &>(*this) = std::move(other);
      return *this;
    }

    SizeLimitedContainer &operator=(
        std::initializer_list<typename Base::value_type> list) {
      if constexpr (size_check_is_enabled) {
        unlikely_if(list.size() > max_size()) {
          throw MaxSizeException(
              "Destination has limited size by {}; Source size is {}",
              max_size(),
              list.size());
        }
      }
      static_cast<Base &>(*this) =
          std::forward<std::initializer_list<typename Base::value_type>>(list);
      return *this;
    }

    void assign(typename Base::size_type size,
                const typename Base::value_type &value) {
      if constexpr (size_check_is_enabled) {
        unlikely_if(size > max_size()) {
          throw MaxSizeException(
              "Destination has limited size by {}; Requested size is {}",
              max_size(),
              size);
        }
      }
      return Base::assign(size, value);
    }

    template <typename Iter,
              typename = std::enable_if_t<std::is_base_of_v<
                  std::input_iterator_tag,
                  typename std::iterator_traits<Iter>::iterator_category>>>
    void assign(Iter begin, Iter end) {
      if constexpr (size_check_is_enabled) {
        const size_t size = std::distance(begin, end);
        unlikely_if(size > max_size()) {
          throw MaxSizeException(
              "Container has limited size by {}; Source range size is {}",
              max_size(),
              Base::size());
        }
      }
      return Base::assign(std::move(begin), std::move(end));
    }

    void assign(std::initializer_list<typename Base::value_type> list) {
      if constexpr (size_check_is_enabled) {
        unlikely_if(list.size() > max_size()) {
          throw MaxSizeException(
              "Container has limited size by {}; Source range size is {}",
              max_size(),
              Base::size());
        }
      }
      return Base::assign(std::move(list));
    }

    template <typename... Args>
    typename Base::reference &emplace_back(Args &&...args) {
      if constexpr (size_check_is_enabled) {
        unlikely_if(Base::size() >= max_size()) {
          throw MaxSizeException(
              "Container has limited size by {}; Size is already {} ",
              max_size(),
              Base::size());
        }
      }
      return Base::emplace_back(std::forward<Args>(args)...);
    }

    template <
        typename Iter,
        typename... Args,
        bool isIter = std::is_same_v<Iter, typename Base::iterator>,
        bool isConstIter = std::is_same_v<Iter, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>>
    typename Base::iterator emplace(Iter pos, Args &&...args) {
      if constexpr (size_check_is_enabled) {
        unlikely_if(Base::size() >= max_size()) {
          throw MaxSizeException(
              "Container has limited size by {}; Size is already {} ",
              max_size(),
              Base::size());
        }
      }
      return Base::emplace(std::move(pos), std::forward<Args>(args)...);
    }

    template <
        typename Iter,
        bool isIter = std::is_same_v<Iter, typename Base::iterator>,
        bool isConstIter = std::is_same_v<Iter, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>>
    typename Base::iterator insert(Iter pos,
                                   const typename Base::value_type &value) {
      if constexpr (size_check_is_enabled) {
        unlikely_if(Base::size() >= max_size()) {
          throw MaxSizeException(
              "Destination has limited size by {}; Size is already {} ",
              max_size(),
              Base::size());
        }
      }
      return Base::insert(std::move(pos), value);
    }

    template <
        typename Iter,
        bool isIter = std::is_same_v<Iter, typename Base::iterator>,
        bool isConstIter = std::is_same_v<Iter, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>>
    typename Base::iterator insert(Iter pos,
                                   typename Base::size_type size,
                                   const typename Base::value_type &value) {
      if constexpr (size_check_is_enabled) {
        const auto available = max_size() - Base::size();
        unlikely_if(available < size) {
          throw MaxSizeException(
              "Destination has limited size by {}; Requested size is {}",
              max_size(),
              size);
        }
      }
      return Base::insert(std::move(pos), size, value);
    }

    template <
        typename OutIt,
        typename InIt,
        bool isIter = std::is_same_v<OutIt, typename Base::iterator>,
        bool isConstIter = std::is_same_v<OutIt, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>,
        typename = std::enable_if_t<std::is_base_of_v<
            std::input_iterator_tag,
            typename std::iterator_traits<InIt>::iterator_category>>>
    typename Base::iterator insert(OutIt pos, InIt begin, InIt end) {
      if constexpr (size_check_is_enabled) {
        const size_t size = std::distance(begin, end);
        const auto available = max_size() - Base::size();
        unlikely_if(available < size) {
          throw MaxSizeException(
              "Destination has limited size by {} and current size is {}; "
              "Source range size is {} and would overflow destination",
              max_size(),
              Base::size(),
              size);
        }
      }
      return Base::insert(std::move(pos), std::move(begin), std::move(end));
    }

    template <
        typename Iter,
        bool isIter = std::is_same_v<Iter, typename Base::iterator>,
        bool isConstIter = std::is_same_v<Iter, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>>
    typename Base::iterator insert(
        Iter pos, std::initializer_list<typename Base::value_type> &&list) {
      if constexpr (size_check_is_enabled) {
        const auto available = max_size() - Base::size();
        unlikely_if(available < list.size()) {
          throw MaxSizeException(
              "Container has limited size by {}; Source range size is {}",
              max_size(),
              Base::size());
        }
      }
      return Base::insert(pos, std::move(list));
    }

    template <typename V>
    void push_back(V &&value) {
      if constexpr (size_check_is_enabled) {
        unlikely_if(Base::size() >= max_size()) {
          throw MaxSizeException(
              "Container has limited size by {}; Size is already maximum",
              max_size());
        }
      }
      Base::push_back(std::forward<V>(value));
    }

    void reserve(typename Base::size_type size) {
      if constexpr (size_check_is_enabled) {
        unlikely_if(size > max_size()) {
          throw MaxSizeException(
              "Destination has limited size by {}; Requested size is {}",
              max_size(),
              size);
        }
      }
      return Base::reserve(size);
    }

    void resize(typename Base::size_type size) {
      if constexpr (size_check_is_enabled) {
        unlikely_if(size > max_size()) {
          throw MaxSizeException(
              "Destination has limited size by {}; Requested size is {}",
              max_size(),
              size);
        }
      }
      return Base::resize(size);
    }

    void resize(typename Base::size_type size,
                const typename Base::value_type &value) {
      if constexpr (size_check_is_enabled) {
        unlikely_if(size > max_size()) {
          throw MaxSizeException(
              "Destination has limited size by {}; Requested size is {}",
              max_size(),
              size);
        }
      }
      return Base::resize(size, value);
    }

    auto operator<=>(const SizeLimitedContainer &other) const {
      return cxx20::lexicographical_compare_three_way(
          Base::cbegin(), Base::cend(), other.cbegin(), other.cend());
    }
  };

  template <typename ElementType, size_t MaxSize, typename... Args>
  using SLVector =
      SizeLimitedContainer<std::vector<ElementType, Args...>, MaxSize>;

}  // namespace kagome::common
