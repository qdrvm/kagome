/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <zstd_errors.h>

#include <qtils/enum_error_code.hpp>

namespace kagome::network {
enum class ZstdStreamCompressorError {
    UNKNOWN,
    EXCEPTION,
    UNKNOWN_EXCEPTION,
    CONTEXT_ERROR,
    ERROR_GENERIC,
    PREFIX_UNKNOWN,
    VERSION_UNSUPPORTED,
    PARAMETER_UNKNOWN,
    FRAME_PARAMETER_UNSUPPORTED,
    FRAME_PARAMETER_WINDOW_TOO_LARGE,
    COMPRESSION_PARAMETER_UNSUPPORTED,
    INIT_MISSING,
    MEMORY_ALLOCATION,
    STAGE_WRONG,
    DST_SIZE_TOO_SMALL,
    SRC_SIZE_WRONG,
    CORRUPTION_DETECTED,
    CHECKSUM_WRONG,
    TABLE_LOG_TOO_LARGE,
    MAX_SYMBOL_VALUE_TOO_LARGE,
    MAX_SYMBOL_VALUE_TOO_SMALL,
    DICTIONARY_CORRUPTED,
    DICTIONARY_WRONG,
    DICTIONARY_CREATION_FAILED,
    MAX_CODE,
};

ZstdStreamCompressorError convertErrorCode(ZSTD_ErrorCode errorCode) {
    switch (errorCode) {
        case ZSTD_error_no_error:
            return ZstdStreamCompressorError::UNKNOWN;
        case ZSTD_error_GENERIC:
            return ZstdStreamCompressorError::ERROR_GENERIC;
        case ZSTD_error_prefix_unknown:
            return ZstdStreamCompressorError::PREFIX_UNKNOWN;
        case ZSTD_error_version_unsupported:
            return ZstdStreamCompressorError::VERSION_UNSUPPORTED;
        case ZSTD_error_parameter_unsupported:
            return ZstdStreamCompressorError::COMPRESSION_PARAMETER_UNSUPPORTED;
        case ZSTD_error_frameParameter_unsupported:
            return ZstdStreamCompressorError::FRAME_PARAMETER_UNSUPPORTED;
        case ZSTD_error_frameParameter_windowTooLarge:
            return ZstdStreamCompressorError::FRAME_PARAMETER_WINDOW_TOO_LARGE;
        case ZSTD_error_init_missing:
            return ZstdStreamCompressorError::INIT_MISSING;
        case ZSTD_error_memory_allocation:
            return ZstdStreamCompressorError::MEMORY_ALLOCATION;
        case ZSTD_error_stage_wrong:
            return ZstdStreamCompressorError::STAGE_WRONG;
        case ZSTD_error_dstSize_tooSmall:
            return ZstdStreamCompressorError::DST_SIZE_TOO_SMALL;
        case ZSTD_error_srcSize_wrong:
            return ZstdStreamCompressorError::SRC_SIZE_WRONG;
        case ZSTD_error_corruption_detected:
            return ZstdStreamCompressorError::CORRUPTION_DETECTED;
        case ZSTD_error_checksum_wrong:
            return ZstdStreamCompressorError::CHECKSUM_WRONG;
        case ZSTD_error_tableLog_tooLarge:
            return ZstdStreamCompressorError::TABLE_LOG_TOO_LARGE;
        case ZSTD_error_maxSymbolValue_tooLarge:
            return ZstdStreamCompressorError::MAX_SYMBOL_VALUE_TOO_LARGE;
        case ZSTD_error_maxSymbolValue_tooSmall:
            return ZstdStreamCompressorError::MAX_SYMBOL_VALUE_TOO_SMALL;
        case ZSTD_error_dictionary_corrupted:
            return ZstdStreamCompressorError::DICTIONARY_CORRUPTED;
        case ZSTD_error_dictionary_wrong:
            return ZstdStreamCompressorError::DICTIONARY_WRONG;
        case ZSTD_error_dictionaryCreation_failed:
            return ZstdStreamCompressorError::DICTIONARY_CREATION_FAILED;
        case ZSTD_error_maxCode:
            return ZstdStreamCompressorError::MAX_CODE;
        default:
            return ZstdStreamCompressorError::UNKNOWN;
    }
}

Q_ENUM_ERROR_CODE(ZstdStreamCompressorError) {
    using E = decltype(e);
    switch (e) {
        case E::UNKNOWN:
            return "Unknown error";
        case E::CONTEXT_ERROR:
            return "Failed to create ZSTD compression context";
        case E::ERROR_GENERIC:
            return "Generic error";
        case E::PREFIX_UNKNOWN:
            return "Unknown prefix";
        case E::VERSION_UNSUPPORTED:
            return "Unsupported version";
        case E::PARAMETER_UNKNOWN:
            return "Unknown parameter";
        case E::FRAME_PARAMETER_UNSUPPORTED:
            return "Unsupported frame parameter";
        case E::FRAME_PARAMETER_WINDOW_TOO_LARGE:
            return "Frame parameter window too large";
        case E::COMPRESSION_PARAMETER_UNSUPPORTED:
            return "Unsupported compression parameter";
        case E::INIT_MISSING:
            return "Init missing";
        case E::MEMORY_ALLOCATION:
            return "Memory allocation error";
        case E::STAGE_WRONG:
            return "Wrong stage";
        case E::DST_SIZE_TOO_SMALL:
            return "Destination size too small";
        case E::SRC_SIZE_WRONG:
            return "Wrong source size";
        case E::CORRUPTION_DETECTED:
            return "Corruption detected";
        case E::CHECKSUM_WRONG:
            return "Wrong checksum";
        case E::TABLE_LOG_TOO_LARGE:
            return "Table log too large";
        case E::MAX_SYMBOL_VALUE_TOO_LARGE:
            return "Max symbol value too large";
        case E::MAX_SYMBOL_VALUE_TOO_SMALL:
            return "Max symbol value too small";
        case E::DICTIONARY_CORRUPTED:
            return "Dictionary corrupted";
        case E::DICTIONARY_WRONG:
            return "Wrong dictionary";
        case E::DICTIONARY_CREATION_FAILED:
            return "Dictionary creation failed";
        case E::MAX_CODE:
            return "Max code";
        case E::EXCEPTION:
            return "Exception";
        case E::UNKNOWN_EXCEPTION:
            return "Unknown exception";
        default:
            return "Unknown error";
    }
}

}  // namespace kagome::network
