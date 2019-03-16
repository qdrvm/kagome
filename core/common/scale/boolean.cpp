/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/scale/boolean.hpp"

namespace kagome::common::scale::boolean {
    ByteArray encodeBool(bool value) {
        uint8_t byte = (value ? 0x01 : 0x00);
        return {byte};
    }

    DecodeBoolResult decodeBool(Stream &stream) {
        auto byte = stream.nextByte();
        if (!byte.has_value()) {
            return expected::Error{DecodeError::kNotEnoughData};
        }

        switch (*byte) {
            case 0:
                return expected::Value{false};
            case 1:
                return expected::Value{true};
            default:
                break;
        }

        return expected::Error{DecodeError::kUnexpectedValue};
    }

    uint8_t encodeTribool(tribool value) {
        if (value) {
            return static_cast<uint8_t>(1);
        }

        if (!value) {
            return static_cast<uint8_t>(0);
        }

        return static_cast<uint8_t>(2);
    }

    DecodeTriboolResult decodeTribool(Stream &stream) {
        auto byte = stream.nextByte();
        if (!byte.has_value()) {
            return expected::Error{DecodeError::kNotEnoughData};
        }

        switch (*byte) {
            case 0:
                return expected::Value{false};
            case 1:
                return expected::Value{true};
            case 2:
                return expected::Value{indeterminate};
            default:
                break;
        }

        return expected::Error{DecodeError::kUnexpectedValue};
    }
}
