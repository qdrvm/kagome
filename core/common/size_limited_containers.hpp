/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_SIZELIMITEDCONTAINER
#define KAGOME_COMMON_SIZELIMITEDCONTAINER

#include <fmt/format.h>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#ifdef __has_builtin
#if __has_builtin(__builtin_expect)
#define unlikely(x) __builtin_expect((x), 0)
#endif
#endif
#ifndef unlikely
#define unlikely(x) (x)
#endif

namespace kagome::common {

  class MaxSizeException : public std::length_error {
   public:
    template <typename... Args>
    MaxSizeException(Args &&...args)
        : std::length_error(fmt::format(std::forward<Args>(args)...)) {}
  };

  template <typename BaseContainer, std::size_t MaxSize>
  class SizeLimitedContainer : public BaseContainer {
   protected:
    using Base = BaseContainer;

    static constexpr bool size_check_is_enabled =
        MaxSize < std::numeric_limits<typename Base::size_type>::max();

   public:
    // Next line is required at least for the scale-codec
    static constexpr bool is_static_collection = false;

    [[nodiscard]] inline static constexpr typename Base::size_type max_size() {
      return MaxSize;
    }

    SizeLimitedContainer() = default;

    SizeLimitedContainer(size_t size)
        : Base([&] {
            if constexpr (size_check_is_enabled) {
              if (unlikely(size > max_size())) {
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
              if (unlikely(size > max_size())) {
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
              if (unlikely(other.size() > max_size())) {
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
              if (unlikely(other.size() > max_size())) {
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
              if (unlikely(size > max_size())) {
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
        if (unlikely(other.size() > max_size())) {
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
        if (unlikely(other.size() > max_size())) {
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
        if (unlikely(list.size() > max_size())) {
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
        if (unlikely(size > max_size())) {
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
        if (unlikely(size > max_size())) {
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
        if (unlikely(list.size() > max_size())) {
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
        if (unlikely(Base::size() >= max_size())) {
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
        if (unlikely(Base::size() >= max_size())) {
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
        if (unlikely(Base::size() >= max_size())) {
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
        if (unlikely(available < size)) {
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
        if (unlikely(available < size)) {
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
        if (unlikely(available < list.size())) {
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
        if (unlikely(Base::size() >= max_size())) {
          throw MaxSizeException(
              "Container has limited size by {}; Size is already maximum",
              max_size());
        }
      }
      Base::push_back(std::forward<V>(value));
    }

    void reserve(typename Base::size_type size) {
      if constexpr (size_check_is_enabled) {
        if (unlikely(size > max_size())) {
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
        if (unlikely(size > max_size())) {
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
        if (unlikely(size > max_size())) {
          throw MaxSizeException(
              "Destination has limited size by {}; Requested size is {}",
              max_size(),
              size);
        }
      }
      return Base::resize(size, value);
    }

    bool operator==(const Base &other) const noexcept {
      return Base::size() == other.size()
             and std::equal(
                 Base::cbegin(), Base::cend(), other.cbegin(), other.cend());
    }

    template <size_t Size>
    bool operator==(const std::array<typename Base::value_type, Size> &other)
        const noexcept {
      return Base::size() == Size
             and std::equal(
                 Base::cbegin(), Base::cend(), other.cbegin(), other.cend());
    }

    bool operator!=(const Base &other) const noexcept {
      return not(*this == other);
    }

    bool operator<(const Base &other) const noexcept {
      return std::lexicographical_compare(
          Base::cbegin(), Base::cend(), other.cbegin(), other.cend());
    }

    template <size_t Size>
    bool operator<(const std::array<typename Base::value_type, Size> &other)
        const noexcept {
      return std::lexicographical_compare(
          Base::cbegin(), Base::cend(), other.cbegin(), other.cend());
    }
  };

  template <typename ElementType, size_t MaxSize, typename... Args>
  using SLVector =
      SizeLimitedContainer<std::vector<ElementType, Args...>, MaxSize>;

}  // namespace kagome::common

#undef unlikely

#endif  // KAGOME_COMMON_SIZELIMITEDCONTAINER
