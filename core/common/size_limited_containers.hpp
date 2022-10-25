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

#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

namespace kagome::common {

  class MaxSizeException : public std::length_error {
   public:
    template <typename... Args>
    MaxSizeException(Args &&...args)
        : std::length_error(fmt::format(std::forward<Args>(args)...)) {}
  };

  template <typename ValueType,
            std::size_t MaxSize,
            template <typename...>
            class BaseContainer,
            typename... Args>
  class SizeLimitedContainer : public BaseContainer<ValueType, Args...> {
   protected:
    using Base = BaseContainer<ValueType, Args...>;
    static constexpr bool size_check_is_disabled =
        MaxSize >= std::numeric_limits<typename Base::size_type>::max();

   public:
    // Next line is required at least for the scale-codec
    static constexpr bool is_static_collection = false;

    [[__nodiscard__]] inline static constexpr typename Base::size_type
    max_size() {
      return MaxSize;
    }

    SizeLimitedContainer() = default;

    SizeLimitedContainer(
        size_t size,
        typename Base::value_type &&value = typename Base::value_type())
        : Base([&] {
            if constexpr (size_check_is_disabled) {
              return Base(size, std::forward<typename Base::value_type>(value));
            } else {
              if (likely(size <= max_size())) {
                return Base(size,
                            std::forward<typename Base::value_type>(value));
              }
              throw MaxSizeException(
                  "Destination has limited size by {}; requested size is {}",
                  max_size(),
                  size);
            }
          }()) {}

    explicit SizeLimitedContainer(const Base &other)
        : Base([&] {
            if constexpr (size_check_is_disabled) {
              return other;
            } else {
              if (likely(other.size() <= max_size())) {
                return other;
              }
              throw MaxSizeException(
                  "Destination has limited size by {}; Source size is {}",
                  max_size(),
                  other.size());
            }
          }()) {}

    explicit SizeLimitedContainer(Base &&other)
        : Base([&] {
            if constexpr (size_check_is_disabled) {
              return std::move(other);
            } else {
              if (likely(other.size() <= max_size())) {
                return std::move(other);
              }
              throw MaxSizeException(
                  "Destination has limited size by {}; Source size is {}",
                  max_size(),
                  other.size());
            }
          }()) {}

    template <typename Iter,
              typename = std::enable_if_t<std::is_base_of_v<
                  std::input_iterator_tag,
                  typename std::iterator_traits<Iter>::iterator_category>>>
    SizeLimitedContainer(Iter begin, Iter end)
        : Base([&] {
            if constexpr (size_check_is_disabled) {
              return Base(std::forward<Iter>(begin), std::forward<Iter>(end));
            } else {
              const size_t size = std::distance(begin, end);
              if (likely(size <= max_size())) {
                return Base(std::forward<Iter>(begin), std::forward<Iter>(end));
              }
              throw MaxSizeException(
                  "Container has limited size by {}; Source range size is {}",
                  max_size(),
                  Base::size());
            }
          }()) {}

    SizeLimitedContainer(std::initializer_list<ValueType> list)
        : Base([&] {
            if constexpr (size_check_is_disabled) {
              return Base(std::forward<std::initializer_list<ValueType>>(list));
            } else if (likely(list.size() <= max_size())) {
              return Base(std::forward<std::initializer_list<ValueType>>(list));
            } else {
              throw MaxSizeException(
                  "Container has limited size by {}; Source range size is {}",
                  max_size(),
                  Base::size());
            }
          }()) {}

    SizeLimitedContainer &operator=(const Base &other) {
      if constexpr (size_check_is_disabled) {
        assign(other.begin(), other.end());
        return *this;
      } else if (std::less_equal<size_t>()(other.size(), max_size())) {
        assign(other.begin(), other.end());
        return *this;
      } else {
        throw MaxSizeException(
            "Destination has limited size by {}; Source size is {}",
            max_size(),
            other.size());
      }
    }

    SizeLimitedContainer &operator=(Base &&other) {
      if constexpr (size_check_is_disabled) {
        static_cast<Base &>(*this) = std::forward<Base>(other);
        return *this;
      } else {
        if (likely(other.size() <= max_size())) {
          static_cast<Base &>(*this) = std::forward<Base>(other);
          return *this;
        }
        throw MaxSizeException(
            "Destination has limited size by {}; Source size is {}",
            max_size(),
            other.size());
      }
    }

    SizeLimitedContainer &operator=(std::initializer_list<ValueType> list) {
      if constexpr (size_check_is_disabled) {
        static_cast<Base &>(*this) =
            std::forward<std::initializer_list<ValueType>>(list);
        return *this;
      } else {
        if (likely(list.size() <= max_size())) {
          static_cast<Base &>(*this) =
              std::forward<std::initializer_list<ValueType>>(list);
          return *this;
        }
        throw MaxSizeException(
            "Destination has limited size by {}; Source size is {}",
            max_size(),
            list.size());
      }
    }

    void assign(typename Base::size_type size, const ValueType &value) {
      if constexpr (size_check_is_disabled) {
        return Base::assign(size, value);
      } else {
        if (likely(size <= max_size())) {
          return Base::assign(size, value);
        }
        throw MaxSizeException(
            "Destination has limited size by {}; Requested size is {}",
            max_size(),
            size);
      }
    }

    template <typename Iter,
              typename = std::enable_if_t<std::is_base_of_v<
                  std::input_iterator_tag,
                  typename std::iterator_traits<Iter>::iterator_category>>>
    void assign(Iter begin, Iter end) {
      if constexpr (size_check_is_disabled) {
        return Base::assign(std::forward<Iter>(begin), std::forward<Iter>(end));
      } else {
        const size_t size = std::distance(begin, end);
        if (likely(size <= max_size())) {
          return Base::assign(std::forward<Iter>(begin),
                              std::forward<Iter>(end));
        }
        throw MaxSizeException(
            "Container has limited size by {}; Source range size is {}",
            max_size(),
            Base::size());
      }
    }

    void assign(std::initializer_list<ValueType> list) {
      if constexpr (size_check_is_disabled) {
        return Base::assign(
            std::forward<std::initializer_list<ValueType>>(list));
      } else {
        if (likely(list.size() <= max_size())) {
          return Base::assign(
              std::forward<std::initializer_list<ValueType>>(list));
        }
        throw MaxSizeException(
            "Container has limited size by {}; Source range size is {}",
            max_size(),
            Base::size());
      }
    }

    template <typename V>
    void emplace_back(V &&value) {
      if constexpr (size_check_is_disabled) {
        Base::emplace_back(std::forward<V>(value));
      } else {
        if (likely(Base::size() < max_size())) {
          Base::emplace_back(std::forward<V>(value));
          return;
        }
        throw MaxSizeException(
            "Container has limited size by {}; Size is already {} ",
            max_size(),
            Base::size());
      }
    }

    template <
        typename Iter,
        typename V,
        bool isIter = std::is_same_v<Iter, typename Base::iterator>,
        bool isConstIter = std::is_same_v<Iter, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>>
    void emplace(Iter &&pos, V &&value) {
      if constexpr (size_check_is_disabled) {
        Base::emplace(std::forward<Iter>(pos), std::forward<V>(value));
      } else {
        if (likely(Base::size() < max_size())) {
          Base::emplace(std::forward<Iter>(pos), std::forward<V>(value));
          return;
        }
        throw MaxSizeException(
            "Container has limited size by {}; Size is already {} ",
            max_size(),
            Base::size());
      }
    }

    template <
        typename Iter,
        bool isIter = std::is_same_v<Iter, typename Base::iterator>,
        bool isConstIter = std::is_same_v<Iter, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>>
    void insert(Iter &&pos, const ValueType &value) {
      if constexpr (size_check_is_disabled) {
        Base::insert(std::forward<Iter>(pos), value);
      } else {
        if (likely(Base::size() < max_size())) {
          Base::insert(std::forward<Iter>(pos), value);
          return;
        }
        throw MaxSizeException(
            "Destination has limited size by {}; Size is already {} ",
            max_size(),
            Base::size());
      }
    }

    template <
        typename Iter,
        bool isIter = std::is_same_v<Iter, typename Base::iterator>,
        bool isConstIter = std::is_same_v<Iter, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>>
    void insert(Iter &&pos,
                typename Base::size_type size,
                const ValueType &value) {
      if constexpr (size_check_is_disabled) {
        Base::insert(std::forward<Iter>(pos), size, value);
      } else {
        const auto available = max_size() - Base::size();
        if (likely(available >= size)) {
          Base::insert(std::forward<Iter>(pos), size, value);
          return;
        }
        throw MaxSizeException(
            "Destination has limited size by {}; Requested size is {}",
            max_size(),
            size);
      }
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
    void insert(OutIt &&pos, InIt &&begin, InIt &&end) {
      if constexpr (size_check_is_disabled) {
        Base::insert(std::forward<OutIt>(pos),
                     std::forward<InIt>(begin),
                     std::forward<InIt>(end));
      } else {
        const size_t size = std::distance(begin, end);
        const auto available = max_size() - Base::size();
        if (likely(available >= size)) {
          Base::insert(std::forward<OutIt>(pos),
                       std::forward<InIt>(begin),
                       std::forward<InIt>(end));
          return;
        }
        throw MaxSizeException(
            "Destination has limited size by {} and current size is {}; "
            "Source range size is {} and would overflow destination",
            max_size(),
            Base::size(),
            size);
      }
    }

    template <
        typename Iter,
        bool isIter = std::is_same_v<Iter, typename Base::iterator>,
        bool isConstIter = std::is_same_v<Iter, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>>
    void insert(Iter &&pos, std::initializer_list<ValueType> &&list) {
      if constexpr (size_check_is_disabled) {
        Base::insert(std::forward<Iter>(pos),
                     std::forward<std::initializer_list<ValueType>>(list));
      } else {
        const auto available = max_size() - Base::size();
        if (likely(available >= list.size())) {
          Base::insert(std::forward<Iter>(pos),
                       std::forward<std::initializer_list<ValueType>>(list));
          return;
        }
        throw MaxSizeException(
            "Container has limited size by {}; Source range size is {}",
            max_size(),
            Base::size());
      }
    }

    template <typename V>
    void push_back(V &&value) {
      if constexpr (size_check_is_disabled) {
        Base::push_back(std::forward<V>(value));
      } else {
        if (likely(Base::size() < max_size())) {
          Base::push_back(std::forward<V>(value));
          return;
        }
        throw MaxSizeException(
            "Container has limited size by {}; Size is already maximum",
            max_size());
      }
    }

    void reserve(typename Base::size_type size) {
      if constexpr (size_check_is_disabled) {
        return Base::reserve(size);
      } else {
        if (likely(size <= max_size())) {
          return Base::reserve(size);
        }
        throw MaxSizeException(
            "Destination has limited size by {}; Requested size is {}",
            max_size(),
            size);
      }
    }

    void resize(typename Base::size_type size) {
      if constexpr (size_check_is_disabled) {
        return Base::resize(size);
      } else {
        if (likely(size <= max_size())) {
          return Base::resize(size);
        }
        throw MaxSizeException(
            "Destination has limited size by {}; Requested size is {}",
            max_size(),
            size);
      }
    }

    void resize(typename Base::size_type size, const ValueType &value) {
      if constexpr (size_check_is_disabled) {
        return Base::resize(size, value);
      } else {
        if (likely(size <= max_size())) {
          return Base::resize(size, value);
        }
        throw MaxSizeException(
            "Destination has limited size by {}; Requested size is {}",
            max_size(),
            size);
      }
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
      SizeLimitedContainer<ElementType, MaxSize, std::vector, Args...>;

}  // namespace kagome::common

#undef likely

#endif  // KAGOME_COMMON_SIZELIMITEDCONTAINER
