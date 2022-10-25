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

   public:
    // Next line is required at least for the scale-codec
    static constexpr bool is_static_collection = false;

    [[__nodiscard__]] inline constexpr std::size_t max_size() const {
      return MaxSize;
    }

    SizeLimitedContainer() = default;

    SizeLimitedContainer(
        size_t size,
        typename Base::value_type &&value = typename Base::value_type())
        : Base([&]() -> Base {
            if (size <= max_size()) {
              return Base(size, std::forward<typename Base::value_type>(value));
            }
            throw MaxSizeException(
                "Destination has limited size by {}; requested size is {}",
                max_size(),
                size);
          }()) {}

    explicit SizeLimitedContainer(const Base &other)
        : Base([&] {
            if (other.size() <= max_size()) {
              return other;
            }
            throw MaxSizeException(
                "Destination has limited size by {}; Source size is {}",
                max_size(),
                other.size());
          }()) {}

    explicit SizeLimitedContainer(Base &&other)
        : Base([&] {
            if (other.size() <= max_size()) {
              return std::move(other);
            }
            throw MaxSizeException(
                "Destination has limited size by {}; Source size is {}",
                max_size(),
                other.size());
          }()) {}

    template <typename Iter,
              typename = std::enable_if_t<std::is_base_of_v<
                  std::input_iterator_tag,
                  typename std::iterator_traits<Iter>::iterator_category>>>
    SizeLimitedContainer(Iter begin, Iter end)
        : Base([&] {
            const size_t size = std::distance(begin, end);
            if (size <= max_size()) {
              return Base(std::forward<Iter>(begin), std::forward<Iter>(end));
            }
            throw MaxSizeException(
                "Container has limited size by {}; Source range size is {}",
                max_size(),
                Base::size());
          }()) {}

    SizeLimitedContainer(std::initializer_list<ValueType> list)
        : Base([&] {
            if (list.size() <= max_size()) {
              return Base(std::forward<std::initializer_list<ValueType>>(list));
            }
            throw MaxSizeException(
                "Container has limited size by {}; Source range size is {}",
                max_size(),
                Base::size());
          }()) {}

    SizeLimitedContainer &operator=(const Base &other) {
      if (std::less_equal<size_t>()(other.size(), max_size())) {
        assign(other.begin(), other.end());
        return *this;
      }
      throw MaxSizeException(
          "Destination has limited size by {}; Source size is {}",
          max_size(),
          other.size());
    }

    SizeLimitedContainer &operator=(Base &&other) {
      if (other.size() <= max_size()) {
        static_cast<Base &>(*this) = std::forward<Base>(other);
        return *this;
      }
      throw MaxSizeException(
          "Destination has limited size by {}; Source size is {}",
          max_size(),
          other.size());
    }

    SizeLimitedContainer &operator=(std::initializer_list<ValueType> list) {
      if (list.size() <= max_size()) {
        static_cast<Base &>(*this) =
            std::forward<std::initializer_list<ValueType>>(list);
        return *this;
      }
      throw MaxSizeException(
          "Destination has limited size by {}; Source size is {}",
          max_size(),
          list.size());
    }

    void assign(std::size_t size, const ValueType &value) {
      if (size <= max_size()) {
        return Base::assign(size, value);
      }
      throw MaxSizeException(
          "Destination has limited size by {}; Requested size is {}",
          max_size(),
          size);
    }

    template <typename Iter,
              typename = std::enable_if_t<std::is_base_of_v<
                  std::input_iterator_tag,
                  typename std::iterator_traits<Iter>::iterator_category>>>
    void assign(Iter begin, Iter end) {
      const size_t size = std::distance(begin, end);
      if (size <= max_size()) {
        return Base::assign(std::forward<Iter>(begin), std::forward<Iter>(end));
      }
      throw MaxSizeException(
          "Container has limited size by {}; Source range size is {}",
          max_size(),
          Base::size());
    }

    void assign(std::initializer_list<ValueType> list) {
      if (list.size() <= max_size()) {
        return Base::assign(
            std::forward<std::initializer_list<ValueType>>(list));
      }
      throw MaxSizeException(
          "Container has limited size by {}; Source range size is {}",
          max_size(),
          Base::size());
    }

    template <typename V>
    void emplace_back(V &&value) {
      if (Base::size() < max_size()) {
        Base::emplace_back(std::forward<V>(value));
        return;
      }
      throw MaxSizeException(
          "Container has limited size by {}; Size is already {} ",
          max_size(),
          Base::size());
    }

    template <
        typename Iter,
        typename V,
        bool isIter = std::is_same_v<Iter, typename Base::iterator>,
        bool isConstIter = std::is_same_v<Iter, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>>
    void emplace(Iter &&pos, V &&value) {
      if (Base::size() < max_size()) {
        Base::emplace(std::forward<Iter>(pos), std::forward<V>(value));
        return;
      }
      throw MaxSizeException(
          "Container has limited size by {}; Size is already {} ",
          max_size(),
          Base::size());
    }

    template <
        typename Iter,
        bool isIter = std::is_same_v<Iter, typename Base::iterator>,
        bool isConstIter = std::is_same_v<Iter, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>>
    void insert(Iter &&pos, const ValueType &value) {
      if (Base::size() < max_size()) {
        Base::insert(std::forward<Iter>(pos), value);
        return;
      }
      throw MaxSizeException(
          "Destination has limited size by {}; Size is already {} ",
          max_size(),
          Base::size());
    }

    template <
        typename Iter,
        bool isIter = std::is_same_v<Iter, typename Base::iterator>,
        bool isConstIter = std::is_same_v<Iter, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>>
    void insert(Iter &&pos, std::size_t size, const ValueType &value) {
      const auto available = max_size() - Base::size();
      if (available >= size) {
        Base::insert(std::forward<Iter>(pos), size, value);
        return;
      }
      throw MaxSizeException(
          "Destination has limited size by {}; Requested size is {}",
          max_size(),
          size);
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
      const size_t size = std::distance(begin, end);
      const auto available = max_size() - Base::size();
      if (available >= size) {
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

    template <
        typename Iter,
        bool isIter = std::is_same_v<Iter, typename Base::iterator>,
        bool isConstIter = std::is_same_v<Iter, typename Base::const_iterator>,
        typename = std::enable_if_t<isIter or isConstIter>>
    void insert(Iter &&pos, std::initializer_list<ValueType> &&list) {
      const auto available = max_size() - Base::size();
      if (available >= list.size()) {
        Base::insert(std::forward<Iter>(pos),
                     std::forward<std::initializer_list<ValueType>>(list));
        return;
      }
      throw MaxSizeException(
          "Container has limited size by {}; Source range size is {}",
          max_size(),
          Base::size());
    }

    template <typename V>
    void push_back(V &&value) {
      if (Base::size() < max_size()) {
        return Base::push_back(std::forward<V>(value));
      }
      throw MaxSizeException(
          "Container has limited size by {}; Size is already maximum",
          max_size());
    }

    void reserve(std::size_t size) {
      if (size <= max_size()) {
        return Base::reserve(size);
      }
      throw MaxSizeException(
          "Destination has limited size by {}; Requested size is {}",
          max_size(),
          size);
    }

    void resize(std::size_t size) {
      if (size <= max_size()) {
        return Base::resize(size);
      }
      throw MaxSizeException(
          "Destination has limited size by {}; Requested size is {}",
          max_size(),
          size);
    }

    void resize(std::size_t size, const ValueType &value) {
      if (size <= max_size()) {
        return Base::resize(size, value);
      }
      throw MaxSizeException(
          "Destination has limited size by {}; Requested size is {}",
          max_size(),
          size);
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

  template <typename ElementType, std::size_t MaxSize, typename... Args>
  using SLVector =
      SizeLimitedContainer<ElementType, MaxSize, std::vector, Args...>;

}  // namespace kagome::common

#endif  // KAGOME_COMMON_SIZELIMITEDCONTAINER
