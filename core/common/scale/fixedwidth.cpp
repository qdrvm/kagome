/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/scale/fixedwidth.hpp"

#include <boost/endian/buffers.hpp>
#include "common/scale/util.hpp"

using namespace kagome::common::scale;
using namespace boost::endian;

namespace kagome::common::scale::fixedwidth {
  ByteArray encodeInt8(int8_t value) {
    return impl::encodeInteger(value);
  }

  ByteArray encodeUInt8(uint8_t value) {
    return {value};
  }

  ByteArray encodeInt16(int16_t value) {
    return impl::encodeInteger(value);
  }

  ByteArray encodeUint16(uint16_t value) {
    return impl::encodeInteger(value);
  }

  ByteArray encodeInt32(int32_t value) {
    return impl::encodeInteger(value);
  }

  ByteArray encodeUint32(uint32_t value) {
    return impl::encodeInteger(value);
  }

  ByteArray encodeInt64(int64_t value) {
    return impl::encodeInteger(value);
  }

  ByteArray encodeUint64(uint64_t value) {
    return impl::encodeInteger(value);
  }

  std::optional<int8_t> decodeInt8(Stream &stream) {
    return impl::decodeInteger<int8_t>(stream);
  }

  std::optional<uint8_t> decodeUint8(Stream &stream) {
    return impl::decodeInteger<uint8_t>(stream);
  }

  std::optional<int16_t> decodeInt16(Stream &stream) {
    return impl::decodeInteger<int16_t>(stream);
  }

  std::optional<uint16_t> decodeUint16(Stream &stream) {
    return impl::decodeInteger<uint16_t>(stream);
  }

  std::optional<int32_t> decodeInt32(Stream &stream) {
    return impl::decodeInteger<int32_t>(stream);
  }

  std::optional<uint32_t> decodeUint32(Stream &stream) {
    return impl::decodeInteger<uint32_t>(stream);
  }

  std::optional<int64_t> decodeInt64(Stream &stream) {
    return impl::decodeInteger<int64_t>(stream);
  }

  std::optional<uint64_t> decodeUint64(Stream &stream) {
    return impl::decodeInteger<uint64_t>(stream);
  }
}  // namespace kagome::common::scale::fixedwidth
