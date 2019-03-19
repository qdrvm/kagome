/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/scale/fixedwidth.hpp"

#include <boost/endian/buffers.hpp>
#include "common/scale/util.hpp"

using namespace kagome::common::scale; // NOLINT
using namespace boost::endian; // NOLINT

namespace kagome::common::scale::fixedwidth {
  void encodeInt8(int8_t value, Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeUInt8(uint8_t value, Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeInt16(int16_t value, Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeUint16(uint16_t value, Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeInt32(int32_t value, Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeUint32(uint32_t value, Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeInt64(int64_t value, Buffer &out) {
    impl::encodeInteger(value, out);
  }

  void encodeUint64(uint64_t value, Buffer &out) {
    impl::encodeInteger(value, out);
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
