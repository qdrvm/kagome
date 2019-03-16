/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_BOOLEAN_HPP
#define KAGOME_SCALE_BOOLEAN_HPP

#include "common/scale/types.hpp"

namespace kagome::common::scale::boolean {
    // booleans
    /**
     * @brief encodeBool encodes bool value
     * @param value source bool value {true, false}
     * @return optional decoded value
     */
    ByteArray encodeBool(bool value);

    /**
     * @brief DecodeBoolResult type is decodeBool function result type
     */
    using DecodeBoolResult = TypeDecodeResult<bool>;

    /**
     * @brief decodeBool decodes bool value
     * @param byte source byte stream
     * @return decoded bool value
     */
    DecodeBoolResult decodeBool(Stream &stream);

    /**
     * @brief encodeTribool encodes tribool value
     * @param value tribool value
     * @return encoded value
     */
    uint8_t encodeTribool(tribool value);

    using DecodeTriboolResult = TypeDecodeResult<tribool>;

    /**
     * @brief decodeTristate decodes tribool value representation
     * @param stream source stream containing tribool value
     * @return  decoded tribool value
     */
    DecodeTriboolResult decodeTribool(Stream &stream);
}

#endif //KAGOME_SCALE_BOOLEAN_HPP
