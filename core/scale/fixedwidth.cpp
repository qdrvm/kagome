/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "fixedwidth.hpp"

#include <boost/endian/buffers.hpp>

#include "scale/util.hpp"

using namespace boost::endian;          // NOLINT

namespace kagome::scale::fixedwidth {
  void encodeInt8(int8_t value, common::Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeUInt8(uint8_t value, common::Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeInt16(int16_t value, common::Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeUint16(uint16_t value, common::Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeInt32(int32_t value, common::Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeUint32(uint32_t value, common::Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeInt64(int64_t value, common::Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeUint64(uint64_t value, common::Buffer &out) {
    impl::encodeInteger(value, out);
  }

  outcome::result<int8_t> decodeInt8(common::ByteStream &stream) {
    return impl::decodeInteger<int8_t>(stream);
  }

  outcome::result<uint8_t> decodeUint8(common::ByteStream &stream) {
    return impl::decodeInteger<uint8_t>(stream);
  }

  outcome::result<int16_t> decodeInt16(common::ByteStream &stream) {
    return impl::decodeInteger<int16_t>(stream);
  }

  outcome::result<uint16_t> decodeUint16(common::ByteStream &stream) {
    return impl::decodeInteger<uint16_t>(stream);
  }

  outcome::result<int32_t> decodeInt32(common::ByteStream &stream) {
    return impl::decodeInteger<int32_t>(stream);
  }

  outcome::result<uint32_t> decodeUint32(common::ByteStream &stream) {
    return impl::decodeInteger<uint32_t>(stream);
  }

  outcome::result<int64_t> decodeInt64(common::ByteStream &stream) {
    return impl::decodeInteger<int64_t>(stream);
  }

  outcome::result<uint64_t> decodeUint64(common::ByteStream &stream) {
    return impl::decodeInteger<uint64_t>(stream);
  }
}  // namespace kagome::common::scale::fixedwidth
