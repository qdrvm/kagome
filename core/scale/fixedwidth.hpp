/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_FIXEDWIDTH_HPP
#define KAGOME_SCALE_FIXEDWIDTH_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "common/byte_stream.hpp"
#include "common/result.hpp"
#include "scale/types.hpp"


namespace kagome::scale::fixedwidth {
  // 8 bit
  /**
   * @brief encodeInt8 encodes int8_t from host- to little- endian
   * @param value source value
   * @return encoded value
   */
  void encodeInt8(int8_t value, common::Buffer &out);

  /**
   * @brief encodeUInt8 encodes uint8_t from host- to little- endian
   * @param value source value
   * @return byte array representing encoded value
   */
  void encodeUInt8(uint8_t value, common::Buffer &out);

  // 16 bit
  /**
   * @brief encodeInt16 encodes int16_t from host- to little- endian
   * @param value source value
   * @return byte array representing encoded value
   */
  void encodeInt16(int16_t value, common::Buffer &out);

  /**
   * @brief encodeUint16 encodes uint16_t from host- to little- endian
   * @param value source value
   * @return byte array representing encoded value
   */
  void encodeUint16(uint16_t value, common::Buffer &out);

  // 32 bit
  /**
   * @brief encodeInt32 encodes int32_t from host- to little- endian
   * @param value source value
   * @return byte array representing encoded value
   */
  void encodeInt32(int32_t value, common::Buffer &out);

  /**
   * @brief encodeUint32 encodes uint32_t from host- to little- endian
   * @param value source value
   * @return byte array representing encoded value
   */
  void encodeUint32(uint32_t value, common::Buffer &out);

  // 64 bit
  /**
   * @brief encodeInt64 encodes int64_t from host- to little- endian
   * @param value source value
   * @return byte array representing encoded value
   */
  void encodeInt64(int64_t value, common::Buffer &out);

  /**
   * @brief encodeInt64 encodes int64_t from host- to little- endian
   * @param value source value
   * @return byte array representing encoded value
   */
  void encodeUint64(uint64_t value, common::Buffer &out);

  // 8 bit
  /**
   * @brief decodeInt8 decodes int8_t from host- to little- endian
   * @param stream source byte stream reference
   * @return optional decoded value
   */
  outcome::result<int8_t> decodeInt8(common::ByteStream &stream);

  /**
   * @brief decodeUint8 decodes uint8_t from host- to little- endian
   * @param stream source byte stream reference
   * @return optional decoded value
   */
  outcome::result<uint8_t> decodeUint8(common::ByteStream &stream);

  // 16 bit
  /**
   * @brief decodeInt16 decodes int16_t from host- to little- endian
   * @param stream source byte stream reference
   * @return optional decoded value
   */
  outcome::result<int16_t> decodeInt16(common::ByteStream &stream);

  /**
   * @brief decodeUint16 decodes uint16_t from host- to little- endian
   * @param stream source byte stream reference
   * @return optional decoded value
   */
  outcome::result<uint16_t> decodeUint16(common::ByteStream &stream);

  // 32 bit
  /**
   * @brief decodeInt32 decodes int32_t from host- to little- endian
   * @param stream source byte stream reference
   * @return optional decoded value
   */
  outcome::result<int32_t> decodeInt32(common::ByteStream &stream);

  /**
   * @brief decodeUint32 decodes uint32_t from host- to little- endian
   * @param stream source byte stream reference
   * @return optional decoded value
   */
  outcome::result<uint32_t> decodeUint32(common::ByteStream &stream);

  // 64 bit
  /**
   * @brief decodeInt64 decodes int64_t from host- to little- endian
   * @param stream source byte stream reference
   * @return optional decoded value
   */
  outcome::result<int64_t> decodeInt64(common::ByteStream &stream);

  /**
   * @brief decodeUint64 decodes uint64_t from host- to little- endian
   * @param stream source byte stream reference
   * @return optional decoded value
   */
  outcome::result<uint64_t> decodeUint64(common::ByteStream &stream);
}  // namespace kagome::scale::fixedwidth

#endif  // KAGOME_SCALE_FIXEDWIDTH_HPP
