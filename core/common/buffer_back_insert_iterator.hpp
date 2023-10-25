/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/buffer.hpp"

/*
 * std::back_insert_iterator is an output iterator
 * that appends to a container for which it was constructed.
 */
template <size_t MaxSize>
class std::back_insert_iterator<kagome::common::SLBuffer<MaxSize>> {
  using Container = kagome::common::SLBuffer<MaxSize>;

 public:
  using value_type = typename Container::value_type;
  using difference_type = typename Container::difference_type;
  using pointer = typename Container::pointer;
  using reference = typename Container::reference;
  using iterator_category = std::random_access_iterator_tag;

  constexpr explicit back_insert_iterator(Container &c) : buf_{c} {}

  back_insert_iterator<Container> &operator=(uint8_t value) {
    buf_.putUint8(value);
    return *this;
  }

  back_insert_iterator<Container> &operator=(uint32_t value) {
    buf_.putUint32(value);
    return *this;
  }

  back_insert_iterator<Container> &operator=(uint64_t value) {
    buf_.putUint64(value);
    return *this;
  }

  back_insert_iterator<Container> &operator=(std::string_view value) {
    buf_.put(value);
    return *this;
  }

  back_insert_iterator<Container> &operator=(std::span<const uint8_t> s) {
    buf_.put(s);
    return *this;
  }

  back_insert_iterator<Container> &operator=(const std::vector<uint8_t> &v) {
    buf_.put(v);
    return *this;
  }

  constexpr back_insert_iterator &operator*() {
    return *this;
  }

  constexpr back_insert_iterator &operator++() {
    return *this;
  }

  constexpr back_insert_iterator &operator++(int) {
    return *this;
  }

 private:
  Container &buf_;
};
