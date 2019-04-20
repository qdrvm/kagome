#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(marshaler
        key_marshaler.hpp
        key_marshaler.cpp
        )
target_link_libraries(marshaler
        buffer
        crypto_error
        crypto_key
        keys_proto
        )
