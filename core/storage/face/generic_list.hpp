/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GENERIC_LIST_HPP
#define KAGOME_GENERIC_LIST_HPP

#include <cstddef>
#include <memory>

#include "storage/face/generic_iterator.hpp"

namespace kagome::face {

  template <typename T>
  class ForwardIterator;

  /// An interface for a generic list
  template <typename T>
  class GenericList {
   public:
    using size_type = size_t;
    using value_type = T;
    using iterator = ForwardIterator<GenericList<T>>;

    virtual ~GenericList() = default;

    /**
     * Put an element to the back of the list
     */
    virtual void push_back(value_type &&t) = 0;
    virtual void push_back(const value_type &t) = 0;

    /**
     * Put an element to the front of the list
     */
    virtual void push_front(value_type &&t) = 0;
    virtual void push_front(const value_type &t) = 0;

    /**
     * Get the back of the list and then remove it
     */
    virtual value_type pop_back() = 0;

    /**
     * Get the front of the list and then remove it
     */
    virtual value_type pop_front() = 0;

    /**
     * Erase a range of elements
     */
    virtual void erase(const iterator &begin, const iterator &end) = 0;

    /*
     * Obtain an iterator to the list
     */
    virtual iterator begin() = 0;
    virtual iterator end() = 0;

    /**
     * Tell whether list is empty
     */
    virtual bool empty() const = 0;

    /**
     * Obtain the size of the list
     */
    virtual size_type size() const = 0;
  };

  /**
   * As GenericIterator is abstract and cannot be instantiated, there is a
   * concrete object that wraps a pointer to a generic iterator
   * @tparam Container over which the iterator would iterate
   */
  template <typename Container>
  class ForwardIterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = ssize_t;
    using pointer = typename Container::value_type *;
    using const_pointer = const typename Container::value_type *;
    using reference = typename Container::value_type &;
    using const_reference = const typename Container::value_type &;
    using value_type = typename Container::value_type;

    ForwardIterator(std::unique_ptr<GenericIterator<Container>> it)
        : it_{std::move(it)} {}

    ForwardIterator(ForwardIterator &&it) noexcept : it_{std::move(it.it_)} {}
    ForwardIterator(ForwardIterator const &it) : it_{it.it_->clone()} {}

    ~ForwardIterator() = default;

    GenericIterator<Container> &get_iterator() {
      return *it_;
    }

    GenericIterator<Container> const &get_iterator() const {
      return *it_;
    }

    ForwardIterator &operator=(const ForwardIterator &it) {
      it_ = it.it_->clone();
      return *this;
    }

    ForwardIterator &operator=(ForwardIterator &&it) noexcept {
      it_ = it.it_->clone();
      return *this;
    }

    bool operator!=(const ForwardIterator &other) {
      return *it_ != *other.it_;
    }

    bool operator==(const ForwardIterator &other) {
      return *it_ == *other.it_;
    }

    reference operator*() const {
      return **it_;
    }

    pointer operator->() {
      return it_->get();
    }

    ForwardIterator &operator++() {
      ++(*it_);
      return *this;
    }

   private:
    std::unique_ptr<GenericIterator<Container>> it_;
  };

}  // namespace kagome::face

#endif  // KAGOME_GENERIC_LIST_HPP
