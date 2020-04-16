/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "common/buffer.hpp"

using kagome::common::Buffer;

class value_type;
namespace std {

/*
 * std::back_insert_iterator is an output iterator
 * that appends to a container for which it was constructed.
 */
template <>
class back_insert_iterator<Buffer> {
public:
  using value_type = Buffer::value_type;

  constexpr explicit back_insert_iterator(Buffer &c):
      buf_ {c} {

  }

  back_insert_iterator<Buffer>&
  operator=(uint8_t value) {
    buf_.putUint8(value);
    return *this;
  }

  back_insert_iterator<Buffer>&
  operator=(uint32_t value) {
    buf_.putUint32(value);
    return *this;
  }

  back_insert_iterator<Buffer>&
  operator=(uint64_t value) {
    buf_.putUint64(value);
    return *this;
  }

  back_insert_iterator<Buffer>&
  operator=(std::string_view value) {
    buf_.put(value);
    return *this;
  }

  back_insert_iterator<Buffer>&
  operator=(gsl::span<const uint8_t> s) {
    buf_.put(s);
    return *this;
  }

  back_insert_iterator<Buffer>&
  operator=(const std::vector<uint8_t>& v) {
    buf_.put(v);
    return *this;
  }

  constexpr back_insert_iterator& operator*() {
    return *this;
  }

  constexpr back_insert_iterator& operator++() {
    return *this;
  }

  constexpr back_insert_iterator& operator++(int) {
    return *this;
  }

private:
  Buffer& buf_;
};

}
