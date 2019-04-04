#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(uvarint
    uvarint.cpp
    uvarint.hpp
    )

target_link_libraries(uvarint
    c_utils
    outcome
    )

add_library(base58_codec
    base58_codec.hpp
    base58_codec.cpp
    )

target_link_libraries(base58_codec
    outcome
    buffer
    )

add_library(ip_v4_validator
    ip_v4_validator.hpp
    ip_v4_validator.cpp
    )

